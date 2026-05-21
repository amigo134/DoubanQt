#include "moviecube.h"
#include "imagecache.h"
#include <QPainter>
#include <QSurfaceFormat>
#include <cmath>

// Vertex data: 6 faces * 4 verts, each: pos(3) + tex(2) + faceIdx(1) = 6 floats
static const GLfloat VERTEX_DATA[] = {
    // Front (+Z) face=0
    -1,-1, 1, 0,1, 0,   1,-1, 1, 1,1, 0,   -1, 1, 1, 0,0, 0,   1, 1, 1, 1,0, 0,
    // Back (-Z) face=1
     1,-1,-1, 0,1, 1,  -1,-1,-1, 1,1, 1,    1, 1,-1, 0,0, 1,  -1, 1,-1, 1,0, 1,
    // Left (-X) face=2
    -1,-1,-1, 0,1, 2,  -1,-1, 1, 1,1, 2,   -1, 1,-1, 0,0, 2,  -1, 1, 1, 1,0, 2,
    // Right (+X) face=3
     1,-1, 1, 0,1, 3,   1,-1,-1, 1,1, 3,    1, 1, 1, 0,0, 3,   1, 1,-1, 1,0, 3,
    // Top (+Y) face=4
    -1, 1, 1, 0,1, 4,   1, 1, 1, 1,1, 4,   -1, 1,-1, 0,0, 4,   1, 1,-1, 1,0, 4,
    // Bottom (-Y) face=5
    -1,-1,-1, 0,1, 5,   1,-1,-1, 1,1, 5,   -1,-1, 1, 0,0, 5,   1,-1, 1, 1,0, 5,
};

static const int VERTEX_STRIDE = 6 * sizeof(GLfloat);

// Vertex shader: attribute/varying syntax for GLSL 110 / ES 100 compatibility
static const char* VERTEX_SHADER =
    "attribute vec3 a_position;\n"
    "attribute vec2 a_texCoord;\n"
    "attribute float a_faceIndex;\n"
    "uniform mat4 u_modelViewProjection;\n"
    "uniform mat3 u_normalMatrix;\n"
    "varying vec2 v_texCoord;\n"
    "varying float v_faceIndex;\n"
    "varying vec3 v_normal;\n"
    "void main() {\n"
    "    gl_Position = u_modelViewProjection * vec4(a_position, 1.0);\n"
    "    v_texCoord = a_texCoord;\n"
    "    v_faceIndex = a_faceIndex;\n"
    "    v_normal = normalize(u_normalMatrix * a_position);\n"
    "}\n";

static const char* FRAGMENT_SHADER =
    "uniform sampler2D u_texture;\n"
    "uniform int u_hoveredFace;\n"
    "varying vec2 v_texCoord;\n"
    "varying float v_faceIndex;\n"
    "varying vec3 v_normal;\n"
    "void main() {\n"
    "    vec4 texColor = texture2D(u_texture, v_texCoord);\n"
    "    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));\n"
    "    float diff = max(dot(normalize(v_normal), lightDir), 0.0);\n"
    "    float ambient = 0.35;\n"
    "    vec4 color = texColor * (ambient + diff * 0.65);\n"
    "    if (u_hoveredFace >= 0 && abs(v_faceIndex - float(u_hoveredFace)) < 0.5) {\n"
    "        color = mix(color, vec4(1.0, 1.0, 0.85, 1.0), 0.25);\n"
    "    }\n"
    "    gl_FragColor = color;\n"
    "}\n";

MovieCube::MovieCube(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
    setMinimumHeight(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QSurfaceFormat fmt = format();
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);
    setFormat(fmt);

    // initial tilt: lean back slightly so top face is visible
    m_orientation = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -20.0f);

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        if (!m_dragging) {
            // auto-rotate around world Y (screen vertical axis)
            QQuaternion delta = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 0.3f);
            m_orientation = delta * m_orientation;
        }
        update();
    });
    m_timer->start();
}

MovieCube::~MovieCube()
{
    makeCurrent();
    delete m_program;
    delete m_vbo;
    delete m_vao;
    for (int i = 0; i < NUM_FACES; ++i)
        delete m_faceTextures[i];
    delete m_defaultTex;
    doneCurrent();
}

void MovieCube::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.96f, 0.965f, 0.973f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    setupShaders();
    setupGeometry();

    delete m_defaultTex;
    m_defaultTex = createDefaultTexture();

    for (int i = 0; i < NUM_FACES; ++i) {
        delete m_faceTextures[i];
        m_faceTextures[i] = nullptr;
        if (!m_faceImages[i].isNull())
            m_faceTextures[i] = createTextureFromImage(m_faceImages[i]);
    }

    // view matrix: camera at (0, 0, 4.5), looking at origin
    m_view.setToIdentity();
    m_view.lookAt(QVector3D(0, 0, 4.5f), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
}

void MovieCube::resizeGL(int w, int h)
{
    updateProjection();
}

void MovieCube::updateProjection()
{
    float aspect = float(width()) / qMax(1, height());
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
}

void MovieCube::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_program || !m_program->isLinked())
        return;

    // Upload dirty textures (must be done in paintGL with current context)
    if (m_textureDirty) {
        for (int i = 0; i < NUM_FACES; ++i) {
            if (!m_faceImages[i].isNull()) {
                delete m_faceTextures[i];
                m_faceTextures[i] = createTextureFromImage(m_faceImages[i]);
            }
        }
        m_textureDirty = false;
    }

    // Compute model matrix from quaternion (world-space rotation)
    QMatrix4x4 model;
    model.rotate(m_orientation);

    QMatrix4x4 mv = m_view * model;
    QMatrix4x4 mvp = m_projection * mv;
    QMatrix3x3 normalMat = mv.normalMatrix();

    // Update hovered face
    m_hoveredFace = pickFace(m_mousePos);

    m_program->bind();
    m_program->setUniformValue("u_modelViewProjection", mvp);
    m_program->setUniformValue("u_normalMatrix", normalMat);
    m_program->setUniformValue("u_texture", 0);
    m_program->setUniformValue("u_hoveredFace", m_hoveredFace);

    m_vao->bind();

    for (int face = 0; face < NUM_FACES; ++face) {
        QOpenGLTexture* tex = m_faceTextures[face] ? m_faceTextures[face] : m_defaultTex;
        tex->bind(0);
        glDrawArrays(GL_TRIANGLE_STRIP, face * 4, 4);
        tex->release();
    }

    m_vao->release();
    m_program->release();
}

void MovieCube::setupShaders()
{
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, FRAGMENT_SHADER);

    m_program->bindAttributeLocation("a_position", 0);
    m_program->bindAttributeLocation("a_texCoord", 1);
    m_program->bindAttributeLocation("a_faceIndex", 2);

    m_program->link();
}

void MovieCube::setupGeometry()
{
    m_vao = new QOpenGLVertexArrayObject(this);
    m_vao->create();
    m_vao->bind();

    m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo->create();
    m_vbo->bind();
    m_vbo->allocate(VERTEX_DATA, sizeof(VERTEX_DATA));

    // position
    m_program->enableAttributeArray(0);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, VERTEX_STRIDE);
    // texCoord
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(GLfloat), 2, VERTEX_STRIDE);
    // faceIndex
    m_program->enableAttributeArray(2);
    m_program->setAttributeBuffer(2, GL_FLOAT, 5 * sizeof(GLfloat), 1, VERTEX_STRIDE);

    m_vao->release();
}

int MovieCube::pickFace(const QPoint& mousePos)
{
    if (mousePos.isNull())
        return -1;
    if (width() <= 0 || height() <= 0)
        return -1;

    // Mouse to NDC
    float mx = 2.0f * mousePos.x() / width() - 1.0f;
    float my = 1.0f - 2.0f * mousePos.y() / height();

    // Transform: apply quaternion rotation
    QMatrix4x4 model;
    model.rotate(m_orientation);

    QMatrix4x4 mat = m_projection * m_view * model;

    int bestFace = -1;
    float bestDist = 0.45f; // threshold in NDC

    for (int i = 0; i < NUM_FACES; ++i) {
        QVector4D clip = mat * QVector4D(m_faceCenters[i], 1.0f);
        if (clip.w() <= 0.0001f) continue; // behind camera
        float ndcX = clip.x() / clip.w();
        float ndcY = clip.y() / clip.w();
        float dist = std::sqrt((ndcX - mx) * (ndcX - mx) + (ndcY - my) * (ndcY - my));
        if (dist < bestDist) {
            bestDist = dist;
            bestFace = i;
        }
    }
    return bestFace;
}

QOpenGLTexture* MovieCube::createDefaultTexture()
{
    QImage img(128, 128, QImage::Format_RGBA8888);
    img.fill(QColor(0x2A, 0x2A, 0x32));
    QPainter p(&img);
    p.setPen(QPen(QColor(0x00, 0xB5, 0x1D), 3));
    p.drawRect(4, 4, 119, 119);
    p.setPen(QColor(0x88, 0x88, 0x99));
    p.setFont(QFont("Arial", 11));
    p.drawText(img.rect(), Qt::AlignCenter, "?");
    p.end();
    return createTextureFromImage(img);
}

QOpenGLTexture* MovieCube::createTextureFromImage(const QImage& image)
{
    auto* tex = new QOpenGLTexture(QOpenGLTexture::Target2D);
    tex->create();
    tex->setData(image.mirrored(false, true)); // flip Y for OpenGL
    tex->setMinificationFilter(QOpenGLTexture::Linear);
    tex->setMagnificationFilter(QOpenGLTexture::Linear);
    tex->setWrapMode(QOpenGLTexture::ClampToEdge);
    return tex;
}

void MovieCube::setMovies(const QList<Movie>& movies)
{
    int count = qMin(NUM_FACES, movies.size());
    for (int i = 0; i < count; ++i) {
        QString posterUrl = movies[i].getPoster();
        if (posterUrl.isEmpty())
            continue;

        ImageCache::instance().loadImage(posterUrl, this,
            [this, i](const QPixmap& pixmap) {
                if (!pixmap.isNull()) {
                    m_faceImages[i] = pixmap.toImage().convertToFormat(QImage::Format_RGBA8888);
                    m_textureDirty = true;
                    update();
                }
            });
    }
}

void MovieCube::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QOpenGLWidget::mousePressEvent(event);
}

void MovieCube::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->pos();
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        // Rotate around world axes (screen-space): Y for horizontal, X for vertical
        QQuaternion qY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f,  delta.x() * 0.3f);
        QQuaternion qX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f,  delta.y() * 0.3f);
        m_orientation = qY * qX * m_orientation;
        m_lastMousePos = event->pos();
    } else {
        int face = pickFace(event->pos());
        setCursor(face >= 0 ? Qt::PointingHandCursor : Qt::OpenHandCursor);
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void MovieCube::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void MovieCube::leaveEvent(QEvent* event)
{
    m_hoveredFace = -1;
    m_mousePos = QPoint();
    QOpenGLWidget::leaveEvent(event);
}
