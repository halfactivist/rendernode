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

#include "openglrenderer.h"
#include <QQuickItem>
#include <QDebug>
#include <QDateTime>
#include "customrenderitem.h"

#if QT_CONFIG(opengl)

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLExtraFunctions>

#define VBO_ENABLED 0
#define PBO_ENABLED 0

#define ATTRLOC_POSITION 0
#define ATTRLOC_COLOR 1
#define ATTRLOC_TEXCOORD 2

/*
#define TEX_WIDTH 1024
#define TEX_HEIGHT 1024
#define TEX_BUFFER_SIZE (TEX_WIDTH * TEX_HEIGHT * 4)
*/

static GLfloat _initial_colors[12] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f
};

OpenGLRenderNode::OpenGLRenderNode(CustomRenderItem *item)
    : m_item(item)
{
    memcpy( this->colors, _initial_colors, sizeof (_initial_colors) );
}

OpenGLRenderNode::~OpenGLRenderNode()
{
    releaseResources();
}

void OpenGLRenderNode::releaseResources()
{
    delete m_program;
    m_program = nullptr;


    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glDeleteTextures( 1, &this->textureID );
    f->glDeleteBuffers( 1, &this->pboID );
}

void OpenGLRenderNode::init()
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions *ef = QOpenGLContext::currentContext()->extraFunctions();

    /*
    this->mapbufferFunctions = new QOpenGLExtension_OES_mapbuffer();
    this->mapbufferFunctions->initializeOpenGLFunctions();
    */

    QSet<QByteArray> extensions = QOpenGLContext::currentContext()->extensions();

    for( QSet<QByteArray>::iterator i = extensions.begin(); i != extensions.end(); i++ ){
        qDebug() << __FUNCTION__ << *i;
    }

    m_program = new QOpenGLShaderProgram;

    static const char *vertexShaderSource =
        "attribute highp vec4 posAttr;\n"
        "attribute lowp vec4 colAttr;\n"
        "varying lowp vec4 col;\n"
        "attribute mediump vec2 textureCoordinate;\n"
        "varying mediump vec2 v_textureCoordinate;\n"
        "uniform highp mat4 matrix;\n"
        "void main() {\n"
        //"   col = colAttr;\n"
        "   v_textureCoordinate = textureCoordinate;\n"
        "   gl_Position = matrix * posAttr;\n"
        "}\n";

    static const char *fragmentShaderSource =
        "varying lowp vec4 col;\n"
        "uniform lowp float opacity;\n"
        "uniform sampler2D s_texture;\n"
        "varying mediump vec2 v_textureCoordinate;\n"
        "void main() {\n"
        //"   gl_FragColor = col * opacity;\n"
        "   gl_FragColor = texture2D( s_texture, v_textureCoordinate );"
        "}\n";

    m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->bindAttributeLocation("posAttr", ATTRLOC_POSITION );
    m_program->bindAttributeLocation("colAttr", ATTRLOC_COLOR );
    m_program->bindAttributeLocation("textureCoordinate", ATTRLOC_TEXCOORD );
    m_program->link();

    m_matrixUniform = m_program->uniformLocation("matrix");
    m_opacityUniform = m_program->uniformLocation("opacity");

    const int VERTEX_SIZE = 6 * sizeof(GLfloat);





}

void OpenGLRenderNode::render(const RenderState *state)
{
    //qDebug() << __FUNCTION__ << "@" << QDateTime::currentMSecsSinceEpoch();

    if (!m_program)
        init();

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    QOpenGLExtraFunctions *ef = QOpenGLContext::currentContext()->extraFunctions();

    m_program->bind();
    m_program->setUniformValue(m_matrixUniform, *state->projectionMatrix() * *matrix());
    m_program->setUniformValue(m_opacityUniform, float(inheritedOpacity()));

    QPointF p0(0, 0);
    QPointF p1(0, m_item->height() - 1);
    QPointF p2(m_item->width() - 1, m_item->height() - 1);
    QPointF p3(m_item->width() - 1, 0);

    GLfloat vertices[8] = { GLfloat(p0.x()), GLfloat(p0.y()),
                            GLfloat(p1.x()), GLfloat(p1.y()),
                            GLfloat(p2.x()), GLfloat(p2.y()),
                            GLfloat(p3.x()), GLfloat(p3.y())
                          };


    m_program->setAttributeArray( ATTRLOC_POSITION, vertices, 2 );
    m_program->enableAttributeArray( ATTRLOC_POSITION );

    GLfloat texCoords[8] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    m_program->setAttributeArray( ATTRLOC_TEXCOORD, texCoords, 2 );
    m_program->enableAttributeArray( ATTRLOC_TEXCOORD );

    //
    // Get pixel data
    //
    int width = 0, height = 0;
    uint8_t *sourcePixels = this->m_item->getPixelBuffer( width, height );

    if( this->textureWidth != width || this->textureHeight != height ){
        // Create new texture

        // Destroy previous if needed
        if( this->textureID ){
            f->glDeleteTextures( 1, &this->textureID );
        }

        this->textureWidth = width;
        this->textureHeight = height;

        // Create textures
        f->glGenTextures( 1, &this->textureID );
        f->glBindTexture( GL_TEXTURE_2D, this->textureID );

        f->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        f->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        f->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        f->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        f->glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, this->textureWidth, this->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
        f->glBindTexture( GL_TEXTURE_2D, 0 );
#if PBO_ENABLED
        // TODO: PBO ALLOCATION GOES HERE
#endif
    }

    // Fill texture
    f->glBindTexture( GL_TEXTURE_2D, this->textureID );
#if !PBO_ENABLED
    f->glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, this->textureWidth, this->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, sourcePixels );
#else
    //f->glBindBuffer( GL_PIXEL_UNPACK_BUFFER, this->pixelBuffer->bufferId() );
    this->pixelBuffer->bind();
    f->glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, TEX_WIDTH, TEX_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
    f->glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
#endif
    f->glBindTexture( GL_TEXTURE_2D, 0 );


    // Draw textured triangles
    f->glDisable( GL_BLEND );
    f->glBindTexture( GL_TEXTURE_2D, this->textureID );
    f->glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
    f->glBindTexture( GL_TEXTURE_2D, 0 );
}

QSGRenderNode::StateFlags OpenGLRenderNode::changedStates() const
{
    return BlendState;
}

QSGRenderNode::RenderingFlags OpenGLRenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF OpenGLRenderNode::rect() const
{
    return QRect(0, 0, m_item->width(), m_item->height());
}

void OpenGLRenderNode::setColors(float *v)
{
    memcpy( this->colors, v, sizeof (this->colors) );
}

#endif // opengl
