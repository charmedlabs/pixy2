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

#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>

#define VW_ASPECT_RATIO   ((float)316/(float)208)

class QGridLayout;
class QTabWidget;
class UsbCam;
class MainWindow;

struct Layer
{
    Layer(QImage img, bool enable, QString desc)
    {
        m_img = img;
        m_enable = enable;
        m_desc = desc;
    }

    QImage m_img;
    bool m_enable;
    QString m_desc;
};

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    VideoWidget(MainWindow *main);
    ~VideoWidget();

    void handleImage(void **args);
    void clear();

    int activeWidth();
    int activeHeight();

    void setupDialog(QTabWidget *tabs);

    int saveImage(const QString &filename);

    enum InputMode
    {
        NONE, POINT, REGION
    };

protected:
    void paintEvent(QPaintEvent *event);
    virtual int heightForWidth(int w) const;
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent *event);


signals:
    void selection(int x0, int y0, int width, int height);
    void mouseLoc(int x, int y);

public slots:
    void flush();
    void handleImage(QImage image, uchar renderFlags, QString desc="");
    void acceptInput(VideoWidget::InputMode mode); // need the VideoWidget qualifier, otherwise it won't recognize the metatype!

private slots:
    void handleCheckBox();
    void handleTimer();

private:
    void updateDialog();

    MainWindow *m_main;

    std::vector<Layer> m_layers;
    std::vector<Layer> m_renderedLayers;
    std::vector<bool> m_layerEnable;

    int m_width;
    int m_height;
    int m_videoWidth;
    int m_videoHeight;
    int m_xOffset;
    int m_yOffset;
    int m_x0;
    int m_y0;
    int m_sbWidth;
    int m_sbHeight;
    int m_resizeState;
    float m_scale;
    bool m_drag;
    InputMode m_inputMode;
    bool m_selection;
    float m_aspectRatio;
    QTimer m_timer;

    QGridLayout *m_dialogLayout;
    QTabWidget *m_dialogTabs;
    uint m_dialogWidgets;
};


#endif // VIDEOWIDGET_H
