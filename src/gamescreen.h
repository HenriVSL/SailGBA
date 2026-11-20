#ifndef GAMESCREEN_H
#define GAMESCREEN_H

#include <QPointer>
#include <QQuickItem>

#include "emulator.h"


// Draws the emulator's latest frame, aspect-correct and centred in the item.
class GameScreen : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Emulator *source READ emulator WRITE setEmulator NOTIFY emulatorChanged)
    Q_PROPERTY(bool smoothScaling READ smoothScaling WRITE setSmoothScaling NOTIFY smoothScalingChanged)
    Q_PROPERTY(bool integerScaling READ integerScaling WRITE setIntegerScaling NOTIFY integerScalingChanged)
    // The rectangle the picture actually occupies, in item coordinates.
    Q_PROPERTY(QRectF screenRect READ screenRect NOTIFY screenRectChanged)

public:
    explicit GameScreen(QQuickItem *parent = nullptr);

    Emulator *emulator() const { return m_emulator; }
    void setEmulator(Emulator *emulator);
    bool smoothScaling() const { return m_smooth; }
    void setSmoothScaling(bool on);
    bool integerScaling() const { return m_integer; }
    void setIntegerScaling(bool on);
    QRectF screenRect() const { return m_screenRect; }

signals:
    void emulatorChanged();
    void smoothScalingChanged();
    void integerScalingChanged();
    void screenRectChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void updateScreenRect();

    QPointer<Emulator> m_emulator;
    bool m_smooth = false;
    bool m_integer = false;
    QRectF m_screenRect;
};

#endif // GAMESCREEN_H
