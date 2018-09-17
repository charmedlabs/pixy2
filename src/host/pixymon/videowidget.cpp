//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QMetaType>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include "mainwindow.h"
#include "videowidget.h"
#include "console.h"
// experimental
//#include "interpreter.h"
#include "renderer.h"

VideoWidget::VideoWidget(MainWindow *main) : QWidget((QWidget *)main)
{
    qRegisterMetaType<VideoWidget::InputMode>("VideoWidget::InputMode");

    m_main = main;
    m_xOffset=0;
    m_yOffset=0;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_scale = 1.0;
    m_drag = false;
    m_inputMode = NONE;
    m_selection = false;
    m_aspectRatio = VW_ASPECT_RATIO;
    m_dialogTabs = NULL;
    m_resizeState = 0;

    // set size policy--- preferred aspect ratio
    QSizePolicy policy = sizePolicy();
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    setMouseTracking(true);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(handleTimer()));
}

VideoWidget::~VideoWidget()
{
}

void VideoWidget::flush()
{
    m_renderedLayers.clear();
    m_renderedLayers = m_layers;
    m_layers.clear();
    updateDialog();
    repaint();
}


//0: don’t render, just put on image list, first
//RENDER_FLAG_BLEND: tack on to end of image list
//RENDER_FLAG_FLUSH put on image list first, and render
//RENDER_FLAG_BLEND | RENDER_FLAG_FLUSH: tack on end of image list and render

void VideoWidget::handleImage(QImage image, uchar renderFlags, QString desc)
{
    // This code bumps the main window size up (if possible) to conform to the actual frame dimension and aspect ratio
    if (m_resizeState<2 && desc=="Background")
    {
        if (m_resizeState==0)
        {
            // "Blow out" the main window dimension.
            // Make it bigger initially, because actual size is pretty small
            setMinimumSize(image.width()*1.25, image.height()*1.25);
            m_resizeState = 1;
        }
        else if (m_resizeState==1)
        {
            // making minimum size native resolution
            setMinimumSize(image.width(), image.height());
            m_resizeState = 2;
        }
    }

    if (m_layers.size()==0)
    {
        m_videoWidth = image.width();
        m_videoHeight = image.height();
    }
    m_layers.push_back(Layer(image, renderFlags!=0, desc));
    if (renderFlags&RENDER_FLAG_FLUSH)
        flush();
}


void VideoWidget::clear()
{
    m_layers.clear();
    m_renderedLayers.clear();
    m_layerEnable.clear();
    QImage img(m_width, m_height, QImage::Format_RGB32);
    img.fill(0xff000000);

    handleImage(img, RENDER_FLAG_FLUSH);
}

int VideoWidget::activeWidth()
{
    return m_width;
}

int  VideoWidget::activeHeight()
{
    return m_height;
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    unsigned int i;
    m_width = this->width();
    m_height = this->height();
    float war;
    float pmar;
    QPainter p(this); // we could render to a QImage instead of to a widget.  This might take longer, but
    // it would allow us to save off the blended image, e.g. to a file.
    QPixmap bgPixmap;

    if (m_renderedLayers.size()==0)
        return;

    // background pixmap
    bgPixmap = QPixmap::fromImage(m_renderedLayers[0].m_img);

    // calc aspect ratios
    war = (float)m_width/(float)m_height; // widget aspect ratio
    pmar = (float)bgPixmap.width()/(float)bgPixmap.height();

    if (pmar!=m_aspectRatio)
    {
        m_aspectRatio = pmar;
        updateGeometry();
    }

    // set blending mode
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // figure out if we need to offset our rendering rectangle
    if (war>pmar)
    {   // width is greater than video
        m_width = this->height()*pmar;
        m_xOffset = (this->width()-m_width)/2;
        m_yOffset = 0;
    }
    else
    { // height is greater than video
        m_height = this->width()/pmar;
        m_yOffset = (this->height()-m_height)/2;
        m_xOffset = 0;
    }

    // figure out scale between background resolution and active width of widget
    m_scale = (float)m_width/bgPixmap.width();

    if (m_layerEnable.size()!=m_renderedLayers.size())
        setupDialog(NULL);

    // draw black background
    QPixmap bgBlk(bgPixmap.width(), bgPixmap.height());
    bgBlk.fill(0xff000000);
    p.drawPixmap(QRect(m_xOffset, m_yOffset, m_width, m_height), bgBlk);

    // draw background
    if (m_layerEnable[0])
        p.drawPixmap(QRect(m_xOffset, m_yOffset, m_width, m_height), bgPixmap);

    // draw/blend foreground images
    for (i=1; i<m_renderedLayers.size(); i++)
    {
        if (m_layerEnable[i])
            p.drawPixmap(QRect(m_xOffset, m_yOffset, m_width, m_height), QPixmap::fromImage(m_renderedLayers[i].m_img));
    }

    // draw selection rectangle
    if (m_selection)
    {
        p.setBrush(QBrush(QColor(0xff, 0xff, 0xff, 0x20)));
        p.setPen(QPen(QColor(0xff, 0xff, 0xff, 0xff)));
        p.drawRect(m_x0, m_y0, m_sbWidth, m_sbHeight);
    }

    QWidget::paintEvent(event);
}

int VideoWidget::heightForWidth(int w) const
{
    return w/m_aspectRatio;
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    int xs, ys;
    Qt::MouseButtons b = event->buttons();
    int x = event->x();
    int y = event->y();

    // deal with mouse location
    xs = (x-m_xOffset)/m_scale+.5;
    ys = (y-m_yOffset)/m_scale+.5;
    if (0<=xs && xs<m_videoWidth && 0<=ys && ys<m_videoHeight)
    {
        emit mouseLoc(xs, ys);
        m_timer.start(100);
    }
    else // out of active video, send invalid values
        emit mouseLoc(-1, -1);

    if (m_drag==false && b&Qt::LeftButton)
    {
        m_drag = true;
        m_x0 = x;
        m_y0 = y;
    }
    else if (m_drag==true && b==Qt::NoButton)
    {
        m_drag = false;
    }

    if (m_drag && m_inputMode==REGION)
    {
        m_sbWidth = x-m_x0;
        m_sbHeight = y-m_y0;
        // check if we have clicked outside of active region
        if (m_x0-m_xOffset>0 && m_y0-m_yOffset>0 && m_x0<m_xOffset+m_width && m_y0<m_yOffset+m_height)
        {
            m_selection = true;
            // limit drag to within active region
            if (m_x0-m_xOffset+m_sbWidth>m_width)
                m_sbWidth = m_width-m_x0+m_xOffset;
            if (m_y0-m_yOffset+m_sbHeight>m_height)
                m_sbHeight = m_height-m_y0+m_yOffset;
            if (m_x0-m_xOffset+m_sbWidth<0)
                m_sbWidth = -m_x0+m_xOffset;
            if (m_y0-m_yOffset+m_sbHeight<0)
                m_sbHeight = -m_y0+m_yOffset;
            repaint();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    m_selection = false;
    repaint();

    QWidget::mousePressEvent(event);
}

void VideoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    int x, y, width, height;

    if (m_selection)
    {

        x = (m_x0-m_xOffset)/m_scale+.5;
        y = (m_y0-m_yOffset)/m_scale+.5;

        width = m_sbWidth/m_scale+.5;
        height = m_sbHeight/m_scale+.5;

        // deal with box inversion
        if (width<0)
        {
            x += width;
            width = -width;
        }
        if (height<0)
        {
            y += height;
            height = -height;
        }
        emit selection(x, y, width, height);
        acceptInput(NONE);
        //DBG("%d %d %d %d", x, y, width, height);
        m_selection = false;
    }
    else if (m_inputMode==POINT)
    {
        x = (event->x()-m_xOffset)/m_scale+.5;
        y = (event->y()-m_yOffset)/m_scale+.5;
        emit selection(x, y, 0, 0);
        acceptInput(NONE);
    }
    QWidget::mouseReleaseEvent(event);
}

void VideoWidget::resizeEvent(QResizeEvent *event)
{
    m_selection = false;
    QWidget::resizeEvent(event);
}

void VideoWidget::acceptInput(VideoWidget::InputMode mode)
{
    m_inputMode = mode;
    if (mode==REGION || mode==POINT)
        setCursor(Qt::CrossCursor);
    else
    {
        m_selection = false;
        setCursor(Qt::ArrowCursor);
    }
}

void VideoWidget::updateDialog()
{
    uint i;
    QWidget *tab;
    QCheckBox *cbox;
    QLabel *label, *label2=NULL;
    QLayoutItem *item;

    if (m_dialogTabs==NULL)
        return;

    // have labels changed?
    if (m_dialogWidgets==m_renderedLayers.size())
    {
        for (i=0; i<m_dialogWidgets; i++)
        {
           item = m_dialogLayout->itemAtPosition(i, 2);
           if (item)
           {
               label2 = (QLabel *)item->widget();
               if (label2->text()!=m_renderedLayers[i].m_desc)
                   break;
           }
        }
        if (i==m_dialogWidgets)
            return;
    }

    if (m_dialogLayout)
    {
        QLayoutItem *item;
        QWidget *w;

        while((item=m_dialogLayout->takeAt(0)))
        {
            m_dialogLayout->removeItem(item);
            if ((w=item->widget()))
                delete w;
        }
    }
    else
    {
        tab = new QWidget();
        m_dialogLayout = new QGridLayout(tab);
        m_dialogLayout->setRowStretch(100, 1);
        m_dialogLayout->setColumnStretch(100, 1);
        m_dialogTabs->addTab(tab, "Layers");
    }

    m_dialogWidgets = m_renderedLayers.size();
	m_layerEnable.clear();
    for (i=0; i<m_dialogWidgets; i++)
    {
        label = new QLabel(QString("Layer ") + QString::number(i));
        if (m_renderedLayers[i].m_desc!="")
            label2 = new QLabel(m_renderedLayers[i].m_desc);
        //label->setToolTip(tip);
        label->setAlignment(Qt::AlignRight);
        m_dialogLayout->addWidget(label, i, 0);

        cbox = new QCheckBox();
        cbox->setChecked(m_renderedLayers[i].m_enable);
        cbox->setProperty("index", QVariant(i));
        //cbox->setToolTip(tip);
        connect(cbox, SIGNAL(clicked()), this, SLOT(handleCheckBox()));
        m_layerEnable.push_back(m_renderedLayers[i].m_enable);
        m_dialogLayout->addWidget(cbox, i, 1);
        if (label2)
            m_dialogLayout->addWidget(label2, i, 2);
    }
}

void VideoWidget::setupDialog(QTabWidget *tabs)
{
    uint i;

    m_dialogTabs = tabs;
    m_dialogLayout = NULL;
    m_dialogWidgets = 0;
    if (m_dialogTabs)
        updateDialog();
    else
    {
        // layer flags are not permanent, for now they are only applicable when config dialog is up
        // when the dialog goes away, we restore defaults.
        m_layerEnable.clear();

        for (i=0; i<m_renderedLayers.size(); i++)
            m_layerEnable.push_back(m_renderedLayers[i].m_enable);
    }
}

void VideoWidget::handleCheckBox()
{
    uint i;
    QCheckBox *cbox = (QCheckBox *)sender();
    uint index = cbox->property("index").value<uint>();
    bool checked = cbox->isChecked();

    for (i=0; i<m_layerEnable.size(); i++)
    {
        if (i==index)
            m_layerEnable[i] = checked;
    }
    repaint();
}

void VideoWidget::handleTimer()
{
    // if we're not under mouse, send invalid values so coordinates are erased
    if (!this->underMouse())
        emit mouseLoc(-1, -1);
}


int VideoWidget::saveImage(const QString &filename)
{
    QPixmap pixmap = grab(QRect(m_xOffset, m_yOffset, m_width, m_height));
    pixmap.save(filename);
    return 0;
}
