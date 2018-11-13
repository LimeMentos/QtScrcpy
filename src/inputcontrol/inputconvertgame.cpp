#include <QDebug>
#include <QCursor>

#include "inputconvertgame.h"

InputConvertGame::InputConvertGame(QObject* parent) : QObject(parent)
{

}

InputConvertGame::~InputConvertGame()
{

}

void InputConvertGame::mouseEvent(const QMouseEvent *from, const QSize &frameSize, const QSize &showSize)
{
    updateSize(frameSize, showSize);

    // mouse move
    if (processMouseMove(from)) {
        return;
    }

    // mouse click
    if (processMouseClick(from)) {
        return;
    }
    return;
}

void InputConvertGame::wheelEvent(const QWheelEvent *from, const QSize &frameSize, const QSize &showSize)
{
    Q_UNUSED(from);
    updateSize(frameSize, showSize);
}

void InputConvertGame::keyEvent(const QKeyEvent *from, const QSize& frameSize, const QSize& showSize)
{
    updateSize(frameSize, showSize);
    if (!from || from->isAutoRepeat()) {
        return;
    }

    // steer wheel
    if (isSteerWheelKeys(from)) {
        processSteerWheel(from);
        return;
    }

    // key click
    if (processKeyClick(from)) {
        return;
    }
}

void InputConvertGame::updateSize(const QSize &frameSize, const QSize &showSize)
{
    m_frameSize = frameSize;
    m_showSize = showSize;
}

void InputConvertGame::sendTouchDownEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_DOWN);
}

void InputConvertGame::sendTouchMoveEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_MOVE);
}

void InputConvertGame::sendTouchUpEvent(int id, QPointF pos)
{
    sendTouchEvent(id, pos, AMOTION_EVENT_ACTION_UP);
}

void InputConvertGame::sendTouchEvent(int id, QPointF pos, AndroidMotioneventAction action)
{
    if (0 > id || MULTI_TOUCH_MAX_NUM-1 < id) {
        return;
    }
    ControlEvent* controlEvent = new ControlEvent(ControlEvent::CET_TOUCH);
    if (!controlEvent) {
        return;
    }
    controlEvent->setTouchEventData(id, action, QRect(calcAbsolutePos(pos).toPoint(), m_frameSize));
    sendControlEvent(controlEvent);
}

QPointF InputConvertGame::calcAbsolutePos(QPointF relativePos)
{
    QPointF absolutePos;
    absolutePos.setX(m_showSize.width() * relativePos.x());
    absolutePos.setY(m_showSize.height() * relativePos.y());
    // convert pos
    absolutePos.setX(absolutePos.x() * m_frameSize.width() / m_showSize.width());
    absolutePos.setY(absolutePos.y() * m_frameSize.height() / m_showSize.height());
    return absolutePos;
}

int InputConvertGame::attachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (0 == multiTouchID[i]) {
            multiTouchID[i] = key;
            return i;
        }
    }
    return -1;
}

void InputConvertGame::detachTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            multiTouchID[i] = 0;
            return;
        }
    }
}

int InputConvertGame::getTouchID(int key)
{
    for (int i = 0; i < MULTI_TOUCH_MAX_NUM; i++) {
        if (key == multiTouchID[i]) {
            return i;
        }
    }
    return -1;
}

bool InputConvertGame::isSteerWheelKeys(const QKeyEvent *from)
{
    for (int key : m_steerWheelKeys) {
        if (key == from->key()) {
            return true;
        }
    }
    return false;
}

void InputConvertGame::processSteerWheel(const QKeyEvent *from)
{    
    int keyPress1 = -1;
    int keyPress2 = -1;
    int keysNum = updateSteerWheelKeysPress(from, keyPress1, keyPress2);
    bool needMove = false;
    if (QEvent::KeyPress == from->type()) {
        if (1 == keysNum) {
            m_steerWheelFirstTouchKey = from->key();
            int id = attachTouchID(m_steerWheelFirstTouchKey);
            if (-1 == id) {
                return;
            }            
            sendTouchDownEvent(id, m_steerWheelPos);
            needMove = true;
        } else if (2 == keysNum) {
            needMove = true;
        }
    } else if (QEvent::KeyRelease == from->type()){
        if (0 == keysNum) {
            sendTouchUpEvent(getTouchID(m_steerWheelFirstTouchKey), m_steerWheelPos);
            detachTouchID(m_steerWheelFirstTouchKey);
            m_steerWheelFirstTouchKey = 0;
        } else if (1 == keysNum) {
            needMove = true;
        }
    }
    if (needMove) {
        steerWheelMove(keysNum, keyPress1, keyPress2);
    }
}

int InputConvertGame::updateSteerWheelKeysPress(const QKeyEvent *from, int& keyPress1, int& keyPress2)
{
    bool keyPress = false;
    if (QEvent::KeyPress == from->type()) {
        keyPress = true;
    } else if (QEvent::KeyRelease == from->type()) {
        keyPress = false;
    }
    if (from->key() == m_steerWheelKeys[SWD_UP]) {
        m_steerWheelKeysPress[SWD_UP] = keyPress;
    } else if (from->key() == m_steerWheelKeys[SWD_RIGHT]) {
        m_steerWheelKeysPress[SWD_RIGHT] = keyPress;
    } else if (from->key() == m_steerWheelKeys[SWD_DOWN]) {
        m_steerWheelKeysPress[SWD_DOWN] = keyPress;
    } else if (from->key() == m_steerWheelKeys[SWD_LEFT]) {
        m_steerWheelKeysPress[SWD_LEFT] = keyPress;
    }

    int count = 0;
    keyPress1 = -1;
    keyPress2 = -1;
    for (int i = 0; i < 4; i++) {
        if (true == m_steerWheelKeysPress[i]) {
            count++;

            if (-1 == keyPress1) {
                keyPress1 = i;
            } else if (-1 == keyPress2) {
                keyPress2 = i;
            }
        }
    }
    return count;
}

void InputConvertGame::steerWheelMove(int keysNum, int keyPress1, int keyPress2)
{
    if (1 != keysNum && 2 != keysNum) {
        return;
    }
    QPointF movePos = m_steerWheelPos;
    switch (keysNum) {
    case 2:
        if (keyPress2 == SWD_UP) {
            movePos.setY(movePos.y() - m_steerWheelOffset.y());
        } else if (keyPress2 == SWD_RIGHT) {
            movePos.setX(movePos.x() + m_steerWheelOffset.x());
        } else if (keyPress2 == SWD_DOWN) {
            movePos.setY(movePos.y() + m_steerWheelOffset.y());
        } else if (keyPress2 == SWD_LEFT) {
            movePos.setX(movePos.x() - m_steerWheelOffset.x());
        }
    case 1:
        if (keyPress1 == SWD_UP) {
            movePos.setY(movePos.y() - m_steerWheelOffset.y());
        } else if (keyPress1 == SWD_RIGHT) {
            movePos.setX(movePos.x() + m_steerWheelOffset.x());
        } else if (keyPress1 == SWD_DOWN) {
            movePos.setY(movePos.y() + m_steerWheelOffset.y());
        } else if (keyPress1 == SWD_LEFT) {
            movePos.setX(movePos.x() - m_steerWheelOffset.x());
        }
        break;
    }
    sendTouchMoveEvent(getTouchID(m_steerWheelFirstTouchKey), movePos);
}

bool InputConvertGame::processKeyClick(const QKeyEvent *from)
{
    QPointF clickPos;
    switch (from->key()) {
    case Qt::Key_Space: // 跳
        clickPos = QPointF(0.96f, 0.7f);
        break;
    case Qt::Key_M: // 地图
        clickPos = QPointF(0.98f, 0.03f);
        break;
    case Qt::Key_Tab: // 背包
        clickPos = QPointF(0.06f, 0.9f);
        break;
    case Qt::Key_Z: // 趴
        clickPos = QPointF(0.95f, 0.9f);
        break;
    case Qt::Key_C: // 蹲
        clickPos = QPointF(0.86f, 0.92f);
        break;
    case Qt::Key_R: // 换弹
        clickPos = QPointF(0.795f, 0.93f);
        break;
    case Qt::Key_Alt: // 小眼睛
        clickPos = QPointF(0.8f, 0.31f);
        break;
    case Qt::Key_F: // 捡东西1
        clickPos = QPointF(0.7f, 0.34f);
        break;
    case Qt::Key_G: // 捡东西2
        clickPos = QPointF(0.7f, 0.44f);
        break;
    case Qt::Key_H: // 捡东西3
        clickPos = QPointF(0.7f, 0.54f);
        break;
    default:
        return false;
        break;
    }

    if (QEvent::KeyPress == from->type()) {
        int id = attachTouchID(from->key());
        sendTouchDownEvent(id, clickPos);
    } else if (QEvent::KeyRelease == from->type()) {
        sendTouchUpEvent(getTouchID(from->key()), clickPos);
        detachTouchID(from->key());
    }
    return true;
}

bool InputConvertGame::processMouseClick(const QMouseEvent *from)
{
    QPointF clickPos;
    if (Qt::LeftButton == from->button()) {
        clickPos = QPointF(0.86f, 0.72f);
    } else if (Qt::RightButton == from->button()){
        clickPos = QPointF(0.96f, 0.52f);
    } else {
        return false;
    }

    if (QEvent::MouseButtonPress == from->type() || QEvent::MouseButtonDblClick == from->type()) {
        int id = attachTouchID(from->button());
        sendTouchDownEvent(id, clickPos);
    } else if (QEvent::MouseButtonRelease == from->type()) {
        sendTouchUpEvent(getTouchID(from->button()), clickPos);
        detachTouchID(from->button());
    } else {
        return false;
    }
    return true;
}

bool InputConvertGame::processMouseMove(const QMouseEvent *from)
{
    if (QEvent::MouseMove != from->type()) {
        return false;
    }

    mouseMoveStartTouch(from);
    startMouseMoveTimer();

    // move
    // pos
    QPointF pos = from->localPos();
    // convert pos
    pos.setX(pos.x() / m_showSize.width());
    pos.setY(pos.y() / m_showSize.height());
    sendTouchMoveEvent(getTouchID(Qt::ExtraButton24), pos);
    m_mouseMoveLastPos = pos;
    if (pos.x() < 0.1 || pos.x() > 0.9 || pos.y() < 0.1 || pos.y() > 0.9) {
        mouseMoveStopTouch();
        mouseMoveStartTouch(from);
    }

    return true;
}

void InputConvertGame::moveCursorToStart(const QMouseEvent *from)
{
    QPoint localPos = QPoint(m_showSize.width()*m_mouseMoveStartPos.x(), m_showSize.height()*m_mouseMoveStartPos.y());
    QPoint posOffset = from->localPos().toPoint() - localPos;
    QPoint globalPos = from->globalPos();

    globalPos -= posOffset;
    QCursor::setPos(globalPos);
}

void InputConvertGame::startMouseMoveTimer()
{
    stopMouseMoveTimer();
    m_mouseMoveTimer = startTimer(1000);
}

void InputConvertGame::stopMouseMoveTimer()
{
    if (0 != m_mouseMoveTimer) {
        killTimer(m_mouseMoveTimer);
        m_mouseMoveTimer = 0;
    }
}

void InputConvertGame::mouseMoveStartTouch(const QMouseEvent* from)
{
    if (!m_mouseMovePress) {
        moveCursorToStart(from);
        int id = attachTouchID(Qt::ExtraButton24);
        sendTouchDownEvent(id, m_mouseMoveStartPos);
        m_mouseMovePress = true;
    }
}

void InputConvertGame::mouseMoveStopTouch()
{
    if (m_mouseMovePress) {
        sendTouchUpEvent(getTouchID(Qt::ExtraButton24), m_mouseMoveLastPos);
        detachTouchID(Qt::ExtraButton24);
        m_mouseMovePress = false;
    }
}

void InputConvertGame::timerEvent(QTimerEvent *event)
{
    if (m_mouseMoveTimer == event->timerId()) {
        stopMouseMoveTimer();
        mouseMoveStopTouch();
    }
}