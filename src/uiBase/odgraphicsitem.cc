/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odgraphicsitem.cc,v 1.24 2012-04-04 08:17:52 cvskris Exp $";

#include "odgraphicsitem.h"

#include "enums.h"
#include "geometry.h"
#include "pixmap.h"
#include "uifont.h"

#include <math.h>

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QRectF>
#include <QRgb>
#include <QStyleOption>


QRectF ODGraphicsPointItem::boundingRect() const
{
    return highlight_ ? QRectF( -2, -2, 4, 4 ) : QRectF( -1,-1, 2, 2 );
}


void ODGraphicsPointItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem* option,
				 QWidget *widget )
{
    painter->setPen( pen() );
    drawPoint( painter );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}
	

void ODGraphicsPointItem::drawPoint( QPainter* painter )
{
    painter->setPen( pen() );
    QPoint pts[13]; int ptnr = 0;
    #define mSetPt(ox,oy) pts[ptnr].setX(ox); pts[ptnr].setY(oy); ptnr++;
    mSetPt( 0, 0 );
    mSetPt( -1, 0 ); mSetPt( 1, 0 );
    mSetPt( 0, -1 ); mSetPt( 0, 1 );
    if ( highlight_ )
    {
	mSetPt( -1, -1 ); mSetPt( 1, -1 );
	mSetPt( -1, 1 ); mSetPt( 1, 1 );
	mSetPt( 2, 0 ); mSetPt( -2, 0 );
	mSetPt( 0, 2 ); mSetPt( 0, -2 );
    }

    for ( int idx=0; idx<13; idx++ )	
	painter->drawPoint( pts[idx] );
}



ODGraphicsMarkerItem::ODGraphicsMarkerItem()
    : QAbstractGraphicsShapeItem()
    , mstyle_( new MarkerStyle2D() )
    , fill_(false)
{}


ODGraphicsMarkerItem::~ODGraphicsMarkerItem()
{ delete mstyle_; }


void ODGraphicsMarkerItem::setMarkerStyle( const MarkerStyle2D& mstyle )
{
    const char* typestr = MarkerStyle2D::getTypeString( mstyle.type_ );
    if ( mstyle.isVisible() || mstyle.size_ != 0 || !typestr || !*typestr )
	*mstyle_ = mstyle;
}


QRectF ODGraphicsMarkerItem::boundingRect() const
{
    return QRectF( -mstyle_->size_, -mstyle_->size_, 
	    	   2*mstyle_->size_, 2*mstyle_->size_ );
}


void ODGraphicsMarkerItem::paint( QPainter* painter,
				  const QStyleOptionGraphicsItem* option,
				  QWidget* widget )
{
   /* if ( side_ != 0 )
    pErrMsg( "TODO: implement single-sided markers" );
    if ( !mIsZero(angle_,1e-3) )
    pErrMsg( "TODO: implement tilted markers" );*/

    const QPointF p00 = mapToScene( QPointF(0,0) );
    const QPointF d01 = mapToScene( QPointF(0,1) )-p00;
    const QPointF d10 = mapToScene( QPointF(1,0) )-p00;

    const float xdist = Math::Sqrt(d10.x()*d10.x()+d10.y()*d10.y() );
    const float ydist = Math::Sqrt(d01.x()*d01.x()+d01.y()*d01.y() );

    const float szx = mstyle_->size_/xdist;
    const float szy = mstyle_->size_/ydist;

    painter->setPen( pen() );
    if ( fill_ )
	painter->setBrush( QColor(QRgb(fillcolor_.rgb())) );

    drawMarker( *painter, mstyle_->type_, szx, szy );

    if ( option->state & QStyle::State_Selected )
    {
	painter->setPen( QPen(option->palette.text(),1.0,Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsMarkerItem::drawMarker( QPainter& painter,
		    MarkerStyle2D::Type typ, float szx, float szy )
{
    switch ( typ )
    {
	case MarkerStyle2D::Square:
	    painter.drawRect( QRectF(-szx, -szy, 2*szx, 2*szy) );
	    break;
	
	case MarkerStyle2D::Target:
	    szx /=2;
	    szy /=2;
	case MarkerStyle2D::Circle:
	    painter.drawEllipse( QRectF( -szx, -szy, 2*szx, 2*szy) );
	    break;

	case MarkerStyle2D::Cross:
	    painter.drawLine( QLineF(-szx, -szy, +szx, +szy) );
	    painter.drawLine( QLineF(-szx, +szy, +szx, -szy) );
	    break;

	case MarkerStyle2D::HLine:
	    painter.drawLine( QLineF( -szx, 0, +szx, 0 ) );
	    break;

	case MarkerStyle2D::VLine:
	    painter.drawLine( QLineF( 0, -szy, 0, +szy ) );
	    break;

	case MarkerStyle2D::Plus:
	    drawMarker( painter, MarkerStyle2D::HLine, szx, szy );
	    drawMarker( painter, MarkerStyle2D::VLine, szx, szy );
	    break;

	case MarkerStyle2D::Plane:
	    painter.drawRect( QRectF(-3*szx, -szy/2, 6*szx, szy) );
	    break;

	case MarkerStyle2D::Triangle: {
	    QPolygonF triangle;
	    triangle += QPointF( -szx, 0 );
	    triangle += QPointF( 0, -2*szy );
	    triangle += QPointF( +szx, 0 );
	    painter.drawPolygon( triangle );
	    } break;

	case MarkerStyle2D::Arrow:
	    drawMarker( painter, MarkerStyle2D::VLine, 2*szx, 2*szy );
	    drawMarker( painter, MarkerStyle2D::Triangle, -szx, -szy );
	    break;
    }
}


ODGraphicsArrowItem::ODGraphicsArrowItem()
    : QAbstractGraphicsShapeItem()
{
}


QRectF ODGraphicsArrowItem::boundingRect() const
{
    return QRectF( -arrowsz_, -arrowsz_/2, arrowsz_, arrowsz_ );
}


void ODGraphicsArrowItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem* option,
				 QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
    painter->setPen( pen() );
    drawArrow( *painter );

    if (option->state & QStyle::State_Selected)
    {
	painter->setPen( QPen(option->palette.text(),1.0,Qt::DashLine) );
	painter->setBrush( Qt::NoBrush );
	painter->drawRect( boundingRect().adjusted(2,2,-2,-2) );
    }
}


void ODGraphicsArrowItem::drawArrow( QPainter& painter )
{
    setLineStyle( painter, arrowstyle_.linestyle_ );

    QPoint qpointtail( -arrowsz_, 0 );
    QPoint qpointhead( 0, 0 );
    painter.drawLine( qpointtail.x(), qpointtail.y(), qpointhead.x(),
	    	      qpointhead.y() ); 
    if ( arrowstyle_.hasHead() )
	drawArrowHead( painter, qpointhead, qpointtail );
    if ( arrowstyle_.hasTail() )
	drawArrowHead( painter, qpointtail, qpointhead );
}


void ODGraphicsArrowItem::setLineStyle( QPainter& painter, const LineStyle& ls )
{
    pen().setStyle( (Qt::PenStyle)ls.type_ );
    pen().setColor( QColor(QRgb(ls.color_.rgb())) );
    pen().setWidth( ls.width_ );

    painter.setPen( pen() );
}


void ODGraphicsArrowItem::drawArrowHead( QPainter& painter, const QPoint& qpt,
					 const QPoint& comingfrom )
{
    static const float headangfac = .82; // bigger => lines closer to main line

    // In UI, Y is positive downward
    const QPoint relvec( qpt.x() - comingfrom.x(), comingfrom.y() - qpt.y() );
    const double ang( atan2((double)relvec.y(),(double)relvec.x()) );

    const ArrowHeadStyle& headstyle = arrowstyle_.headstyle_;
    if ( headstyle.handedness_ == ArrowHeadStyle::TwoHanded )
    {
	switch ( headstyle.type_ )
	{
	    case ArrowHeadStyle::Square:
	    {
	        TypeSet<QPoint> polypts;
		polypts += qpt;
	        const QPoint pt1 = getEndPoint(qpt,M_PI,headstyle.sz_);
	        const QPoint pt2 = getEndPoint(qpt,-(M_PI),headstyle.sz_);
		polypts += pt1;
		polypts += pt2;
		painter.drawPolygon( polypts.arr(), 3 );
		break;
	    }
	    case ArrowHeadStyle::Cross:
	    {
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,.25),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,.75),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,-.25),headstyle.sz_/2)) );
		painter.drawLine( qpt, QPoint(getEndPoint(qpt,
				  getAddedAngle(ang,-.75),headstyle.sz_/2)) );
		break;
	    }
	    case ArrowHeadStyle::Triangle:
	    case ArrowHeadStyle::Line:
	    {
		const QPoint rightend = getEndPoint( qpt,
		    getAddedAngle( ang,headangfac), headstyle.sz_ );
		const QPoint leftend = getEndPoint( qpt,
		    getAddedAngle( ang,-headangfac), headstyle.sz_ );
		painter.drawLine( qpt, rightend );
		painter.drawLine( qpt, leftend );
		if ( headstyle.type_ == ArrowHeadStyle::Triangle )
		    painter.drawLine( leftend, rightend );
		break;
	    }
	}
    }
}


double ODGraphicsArrowItem::getAddedAngle( double ang, float ratiopi )
{
    ang += ratiopi * M_PI;
    while ( ang < -M_PI ) ang += 2 * M_PI;
    while ( ang > M_PI ) ang -= 2 * M_PI;
    return ang;
}


QPoint ODGraphicsArrowItem::getEndPoint( const QPoint& pt, double angle,
					 double len )
{
    QPoint endpt( pt.x(), pt.y() );
    double delta = len * cos( angle );
    endpt.setX( pt.x() + mNINT(delta) );
    // In UI, Y is positive downward
    delta = -len * sin( angle );
    endpt.setY( pt.y() + mNINT(delta) );
    return endpt;
}


ODGraphicsTextItem::ODGraphicsTextItem()
    : QGraphicsTextItem()
{
}


void ODGraphicsTextItem::setTextAlignment( Alignment alignment )
{
    alignoption_.setAlignment( (Qt::Alignment)alignment.uiValue() );
}


void ODGraphicsTextItem::setText( const char* txt )
{ text_ = txt; }


QRectF ODGraphicsTextItem::boundingRect() const
{
    const uiFont& uifnt = FontList().get(
				FontData::key(FontData::GraphicsSmall ) );
    QFontMetrics fm( uifnt.qFont() );
    QRectF rectf( fm.boundingRect( text_ ) );
    return rectf;
}


void ODGraphicsTextItem::paint( QPainter* painter,
				 const QStyleOptionGraphicsItem *option,
				 QWidget *widget )
{
    painter->setClipRect( option->exposedRect );
    painter->drawText( boundingRect(), text_, alignoption_ );

    if (option->state & QStyle::State_Selected)
    {
	painter->setPen(QPen(option->palette.text(), 1.0, Qt::DashLine));
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(boundingRect().adjusted(2, 2, -2, -2));
    }
}



ODGraphicsPixmapItem::ODGraphicsPixmapItem()
    : QGraphicsPixmapItem()
{}


ODGraphicsPixmapItem::ODGraphicsPixmapItem( const ioPixmap& pm )
    : QGraphicsPixmapItem(*pm.qpixmap())
{}


void ODGraphicsPixmapItem::paint( QPainter* painter,
				  const QStyleOptionGraphicsItem* option,
				  QWidget* widget )
{
    painter->setClipRect( option->exposedRect );
    QGraphicsPixmapItem::paint( painter, option, widget );
}



ODGraphicsPolyLineItem::ODGraphicsPolyLineItem()
    : QAbstractGraphicsShapeItem()
{}


QRectF ODGraphicsPolyLineItem::boundingRect() const
{
    return qpolygon_.boundingRect();
}


void ODGraphicsPolyLineItem::paint( QPainter* painter,
				    const QStyleOptionGraphicsItem* option,
				    QWidget* widget )
{
    painter->setPen( pen() );
    painter->drawPolyline( qpolygon_ );
}
