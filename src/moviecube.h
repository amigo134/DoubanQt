#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QTimer>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>
#include <QImage>
#include "moviemodel.h"

class MovieCube : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit MovieCube(QWidget* parent = nullptr);
    ~MovieCube() override;

    void setMovies(const QList<Movie>& movies);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupShaders();
    void setupGeometry();
    void updateProjection();
    int  pickFace(const QPoint& mousePos);
    QOpenGLTexture* createDefaultTexture();
    QOpenGLTexture* createTextureFromImage(const QImage& image);

    static constexpr int NUM_FACES = 6;
    static constexpr int NUM_VERTS = 24; // 4 per face, triangle strip

    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer* m_vbo = nullptr;
    QOpenGLVertexArrayObject* m_vao = nullptr;

    QOpenGLTexture* m_faceTextures[NUM_FACES] = {};
    QOpenGLTexture* m_defaultTex = nullptr;
    QImage m_faceImages[NUM_FACES];
    bool m_textureDirty = false;

    QTimer* m_timer = nullptr;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QQuaternion m_orientation;       // accumulated rotation in world space

    bool m_dragging = false;
    QPoint m_lastMousePos;
    QPoint m_mousePos;
    int m_hoveredFace = -1;

    QVector3D m_faceCenters[NUM_FACES] = {
        { 0,  0,  1},  // front
        { 0,  0, -1},  // back
        {-1,  0,  0},  // left
        { 1,  0,  0},  // right
        { 0,  1,  0},  // top
        { 0, -1,  0}   // bottom
    };
};
