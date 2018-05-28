#include "httpserver.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>
#include "interpreter.h"
#include "renderer.h"


HttpServer::HttpServer()
{
    m_interpreter = NULL;
    m_server = new QHttpServer(this);
    connect(m_server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handleRequest(QHttpRequest*, QHttpResponse*)));

    m_server->listen(QHostAddress::Any, 8080);
}

HttpServer::~HttpServer()
{
    delete m_server;
}

void HttpServer::handleRequest(QHttpRequest *req, QHttpResponse *resp)
{
    Q_UNUSED(req);

    QString reqPath = req->path();
    reqPath.remove(QRegExp("^[/]*"));

    qDebug() << reqPath;

    //resp->setHeader("Content-Length", QString::number(body.size()));
    resp->writeHead(200);

    if (reqPath=="frame")
    {
        if (m_interpreter)
        {
            QByteArray frame;

            m_interpreter->m_renderer->getSavedFrame(&frame);
            resp->end(frame);
        }
    }
    else
    {
        QDir path = QFileInfo(QCoreApplication::applicationFilePath()).absoluteDir();
        QString file = QFileInfo(path, reqPath).absoluteFilePath();

        QFile qf(file);

        if (!qf.open(QIODevice::ReadOnly))
            return;

        QTextStream ts(&qf);

        resp->end(ts.readAll().toUtf8());
    }
}

