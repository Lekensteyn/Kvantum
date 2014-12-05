// Adapted from Oxygen-Transparent -> oxygenblurhelper.cpp

/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014 <tsujan2000@gmail.com>
 * 
 * Kvantum is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Kvantum is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "blurhelper.h"

#include <QEvent>
#include <QVector>
#include <QPushButton>

#if defined Q_WS_X11 || defined Q_OS_LINUX
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

BlurHelper::BlurHelper (QObject* parent) : QObject( parent )
{
#if defined Q_WS_X11 || defined Q_OS_LINUX
  _atom_blur = XInternAtom (QX11Info::display(), "_KDE_NET_WM_BLUR_BEHIND_REGION", False);
#endif
}
/*************************/
void BlurHelper::registerWidget (QWidget* widget)
{
   /* no need to these conditions again */
   /*if (widget->isWindow()
       && widget->testAttribute (Qt::WA_TranslucentBackground)
       && widget->windowType() != Qt::Desktop
       && !widget->testAttribute (Qt::WA_X11NetWmWindowTypeDesktop)
       && !widget->testAttribute (Qt::WA_PaintOnScreen)
       && !widget->inherits ("KScreenSaver")
       && !widget->inherits ("QTipLabel")
       && !widget->inherits ("QSplashScreen")
       && !widget->windowFlags().testFlag(Qt::FramelessWindowHint))*/

    addEventFilter (widget);

}
/*************************/
void BlurHelper::unregisterWidget (QWidget* widget)
{
  widget->removeEventFilter (this);
  clear (widget);
}
/*************************/
bool BlurHelper::eventFilter (QObject* object, QEvent* event)
{
  switch (event->type())
  {
    case QEvent::Hide: {
      QWidget* widget (qobject_cast<QWidget*>(object));
      if (!widget) break;
      QWidget* window (widget->window());
      _pendingWidgets.insert (window, window);
      delayedUpdate();
      break;
    }

    case QEvent::Show:
    case QEvent::Resize: {
      QWidget* widget (qobject_cast<QWidget*>(object));
      if (!widget) break;
      QWidget* window (widget->window());
      /* do delayed update only for top-level windows */
      _pendingWidgets.insert (window, window);
      delayedUpdate();
      break;
    }

    default: break;
  }

  // never eat events
  return false;
}
/*************************/
QRegion BlurHelper::blurRegion (QWidget* widget) const
{
  if (!widget->isVisible()) return QRegion();

  /* trimming the region isn't good for us */
  return (widget->mask().isEmpty() ? widget->rect() : widget->mask());
}
/*************************/
void BlurHelper::update (QWidget* widget) const
{
#if defined Q_WS_X11 || defined Q_OS_LINUX
  if (!(widget->testAttribute (Qt::WA_WState_Created) || widget->internalWinId()))
    return;

  const QRegion region (blurRegion (widget));
  if (region.isEmpty())
    clear (widget);
  else
  {
    QVector<unsigned long> data;
    foreach (const QRect& rect, region.rects())
    {
      data << rect.x() << rect.y() << rect.width() << rect.height();
    }
    XChangeProperty (QX11Info::display(), widget->winId(),
                     _atom_blur, XA_CARDINAL, 32, PropModeReplace,
                     reinterpret_cast<const unsigned char *>(data.constData()),
                     data.size());
  }
#endif
  // force update
  if (widget->isVisible())
    widget->update();
}
/*************************/
void BlurHelper::clear (QWidget* widget) const
{
#if defined Q_WS_X11 || defined Q_OS_LINUX
  XDeleteProperty (QX11Info::display(), widget->winId(), _atom_blur);
#else
  Q_UNUSED (widget);
#endif
  return;
}