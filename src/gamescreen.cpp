#include "gamescreen.h"

#include "emulator.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#include <cmath>

namespace {

// A texture whose pixels are replaced in place each frame, instead of
// allocating a new texture for every frame the emulator produces.
class FrameTexture : public QSGTexture
{
public:
    ~FrameTexture() override
    {
        if (m_id && QOpenGLContext::currentContext())
            QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &m_id);
    }

    int textureId() const override { return int(m_id); }
    QSize textureSize() const override { return m_image.isNull() ? QSize(1, 1) : m_image.size(); }
    bool hasAlphaChannel() const override { return false; }
    bool hasMipmaps() const override { return false; }

    // Fetches the latest frame; returns true if it changed.
    bool update(const FrameBuffer &frames)
    {
        if (!frames.take(&m_image, &m_serial))
            return false;
        m_dirty = true;
        return true;
    }

    void bind() override
    {
        QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
        if (!m_id) {
            gl->glGenTextures(1, &m_id);
            gl->glBindTexture(GL_TEXTURE_2D, m_id);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            gl->glBindTexture(GL_TEXTURE_2D, m_id);
        }
        if (m_dirty && !m_image.isNull()) {
            if (m_image.size() != m_allocated) {
                gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(), 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, m_image.constBits());
                m_allocated = m_image.size();
            } else {
                gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_image.width(), m_image.height(),
                                    GL_RGBA, GL_UNSIGNED_BYTE, m_image.constBits());
            }
            m_dirty = false;
        }
        updateBindOptions(m_lastFilter != filtering());
        m_lastFilter = filtering();
    }

private:
    GLuint m_id = 0;
    QImage m_image;
    QSize m_allocated;
    quint64 m_serial = 0;
    bool m_dirty = false;
    Filtering m_lastFilter = None;
};

} // namespace

GameScreen::GameScreen(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void GameScreen::setEmulator(Emulator *emulator)
{
    if (m_emulator == emulator)
        return;
    if (m_emulator)
        disconnect(m_emulator, nullptr, this, nullptr);
    m_emulator = emulator;
    if (m_emulator) {
        connect(m_emulator, &Emulator::frameReady, this, &QQuickItem::update);
        connect(m_emulator, &Emulator::frameSizeChanged, this, &GameScreen::updateScreenRect);
    }
    updateScreenRect();
    update();
    emit emulatorChanged();
}

void GameScreen::setSmoothScaling(bool on)
{
    if (m_smooth == on)
        return;
    m_smooth = on;
    update();
    emit smoothScalingChanged();
}

void GameScreen::setIntegerScaling(bool on)
{
    if (m_integer == on)
        return;
    m_integer = on;
    updateScreenRect();
    emit integerScalingChanged();
}

void GameScreen::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    updateScreenRect();
}

void GameScreen::updateScreenRect()
{
    const qreal fw = m_emulator ? m_emulator->frameWidth() : 240;
    const qreal fh = m_emulator ? m_emulator->frameHeight() : 160;
    qreal scale = std::min(width() / fw, height() / fh);
    if (m_integer && scale >= 1)
        scale = std::floor(scale);
    const QSizeF size(fw * scale, fh * scale);
    const QRectF rect(QPointF(std::round((width() - size.width()) / 2), std::round((height() - size.height()) / 2)), size);
    if (rect != m_screenRect) {
        m_screenRect = rect;
        update();
        emit screenRectChanged();
    }
}

QSGNode *GameScreen::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!m_emulator || m_screenRect.isEmpty()) {
        delete node;
        return nullptr;
    }
    if (!node) {
        node = new QSGSimpleTextureNode;
        node->setTexture(new FrameTexture);
        node->setOwnsTexture(true);
    }
    FrameTexture *texture = static_cast<FrameTexture *>(node->texture());
    if (texture->update(m_emulator->frameBuffer()))
        node->markDirty(QSGNode::DirtyMaterial);
    node->setFiltering(m_smooth ? QSGTexture::Linear : QSGTexture::Nearest);
    node->setRect(m_screenRect);
    return node;
}
