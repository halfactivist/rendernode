/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "customrenderitem.h"
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>
#include <QDebug>
#include <QDateTime>

#include "openglrenderer.h"
#include "d3d12renderer.h"
#include "softwarerenderer.h"

CustomRenderItem::CustomRenderItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // Our item shows something so set the flag.
    setFlag(ItemHasContents);
    colors[0] = {255,0,0};
    colors[1] = {0,255,0};
    colors[2] = {0,0,255};
    colors[3] = {255,0,255};

    this->initPixelBuffer( 640, 480 );
}

QSGNode *CustomRenderItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    //qDebug() << __FUNCTION__ << "@" << QDateTime::currentMSecsSinceEpoch();


    OpenGLRenderNode *n = static_cast<OpenGLRenderNode *>(node);
    if (!n) {
        QSGRendererInterface *ri = window()->rendererInterface();
        if (!ri)
            return nullptr;
        switch (ri->graphicsApi()) {
            case QSGRendererInterface::OpenGL:
#if QT_CONFIG(opengl)
                n = new OpenGLRenderNode(this);
                break;
#endif
            default:
                return nullptr;
        }
    }

    this->node = n;

    this->node->markDirty( QSGNode::DirtyMaterial );

    return n;
}

void CustomRenderItem::startAnimation()
{
    QTimer *t = new QTimer( this );
    t->setInterval( 10 );
    connect( t, &QTimer::timeout, this, &CustomRenderItem::animationTick );
    t->start();
}

void CustomRenderItem::animationTick()
{
    if( !this->node ){
        return;
    }

    static int calls = 0;
    calls++;

    for( int i = 0; i < 4; i++ ){
        this->colors[i].R += 5;
    }

    uint8_t *c = (uint8_t *)this->colors;
    float colors[12];

    for( int i = 0; i < 12; i++ ){
        colors[i] = ((float)c[i]) / 255.0f;
    }

    this->node->setColors( colors );

    if( (calls % 100) == 0 ){
        this->initPixelBuffer( this->pixelBufferWidth + 10, this->pixelBufferHeight + 10 );
    }

    uint32_t *texBufAsUInt32 = (uint32_t *)this->pixelBuffer;
    for( int y = 0; y < this->pixelBufferWidth; y++ ){
        uint32_t g = ((*texBufAsUInt32 & 0x0000FF00) + 0x00000500) & 0x0000FF00;
        uint32_t rowValue = (*texBufAsUInt32 & 0xFFFF00FF) | g;
        for( int x = 0; x < this->pixelBufferHeight; x++ ){
            *texBufAsUInt32 = rowValue;
            texBufAsUInt32++;
        }
    }

    this->update();
}

void CustomRenderItem::initPixelBuffer( int width, int height )
{
    if( this->pixelBuffer ){
        free( this->pixelBuffer );
    }

    this->pixelBufferWidth = width;
    this->pixelBufferHeight = height;
    this->pixelBuffer = static_cast<uint8_t *>( malloc( this->pixelBufferWidth * this->pixelBufferHeight * 4 * sizeof(uint8_t) ) );


    uint32_t *texBufAsUInt32 = (uint32_t *)this->pixelBuffer;
    for( int y = 0; y < this->pixelBufferWidth; y++ ){
        uint32_t b = y & 0xFF;
        for( int x = 0; x < this->pixelBufferHeight; x++ ){
            *texBufAsUInt32 = 0x00FF0000 | b;
            texBufAsUInt32++;
        }
    }
}

uint8_t *CustomRenderItem::getPixelBuffer( int &width, int &height )
{
    width = this->pixelBufferWidth;
    height = this->pixelBufferHeight;
    return this->pixelBuffer;
}
