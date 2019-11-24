/***************************************************************************
 *   Project TUPITUBE DESK                                                *
 *   Project Contact: info@maefloresta.com                                 *
 *   Project Website: http://www.maefloresta.com                           *
 *   Project Leader: Gustav Gonzalez <info@maefloresta.com>                *
 *                                                                         *
 *   Developers:                                                           *
 *   2010:                                                                 *
 *    Gustav Gonzalez / xtingray                                           *
 *                                                                         *
 *   KTooN's versions:                                                     * 
 *                                                                         *
 *   2006:                                                                 *
 *    David Cuadrado                                                       *
 *    Jorge Cuadrado                                                       *
 *   2003:                                                                 *
 *    Fernado Roldan                                                       *
 *    Simena Dinas                                                         *
 *                                                                         *
 *   Copyright (C) 2010 Gustav Gonzalez - http://www.maefloresta.com       *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "tupgraphicsscene.h"
#include "tupitemgroup.h"
#include "tupprojectloader.h"
#include "tupitemfactory.h"
#include "tupinputdeviceinformation.h"
#include "tupitemtweener.h"
#include "tupgraphiclibraryitem.h"
#include "tuppathitem.h"
#include "tuplineitem.h"
#include "tuprectitem.h"
#include "tupellipseitem.h"

#include <QSvgRenderer>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QMouseEvent>

TupGraphicsScene::TupGraphicsScene() : QGraphicsScene()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene()";
    #endif

    loadingProject = true;
    // dynamicBg = new QGraphicsPixmapItem;

    setItemIndexMethod(QGraphicsScene::NoIndex);

    framePosition.layer = -1;
    framePosition.frame = -1;
    spaceContext = TupProject::FRAMES_MODE;

    onionSkin.next = 0;
    onionSkin.previous = 0;
    gTool = nullptr;
    isDrawing = false;

    inputInformation = new TupInputDeviceInformation(this);
    brushManager = new TupBrushManager(this);
}

TupGraphicsScene::~TupGraphicsScene()
{
    #ifdef TUP_DEBUG
        qDebug() << "~TupGraphicsScene()";
    #endif

    clearFocus();
    clearSelection();

    // SQA: Check if these instructions are actually required
    foreach (QGraphicsItem *item, items()) {
        removeItem(item);
        delete item;
        item = nullptr;
    }
}

void TupGraphicsScene::updateLayerVisibility(int layerIndex, bool visible)
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupGraphicsScene::updateLayerVisibility()]";
    #endif

    if (!gScene)
        return;

    if (TupLayer *layer = gScene->layerAt(layerIndex))
        layer->setLayerVisibility(visible);
}

void TupGraphicsScene::setCurrentFrame(int layer, int frame)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setCurrentFrame()";
    #endif

    if ((frame != framePosition.frame && framePosition.frame >= 0) ||
        (layer != framePosition.layer && framePosition.layer >= 0)) {
        if (gTool) {
            if (gTool->name().compare(tr("PolyLine")) == 0 || gTool->toolType() == TupToolInterface::Tweener)
                gTool->aboutToChangeScene(this);
        }
    }

    framePosition.layer = layer;
    framePosition.frame = frame;

    foreach (QGraphicsView *view, views())
        view->setDragMode(QGraphicsView::NoDrag);
}

void TupGraphicsScene::drawCurrentPhotogram()
{
    #ifdef TUP_DEBUG
        qDebug() << "[TupGraphicsScene::drawCurrentPhotogram()]";
    #endif

    if (loadingProject)
        return;

    TupLayer *layer = gScene->layerAt(framePosition.layer);
    if (layer) {
        int frames = layer->framesCount();

        if (framePosition.frame >= frames)
            framePosition.frame = frames - 1;

        if (spaceContext == TupProject::FRAMES_MODE) {
            drawPhotogram(framePosition.frame, true);
        } else {
            cleanWorkSpace();
            drawSceneBackground(framePosition.frame);
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::drawCurrentPhotogram() - Fatal error: Invalid layer index -> "
                        + QString::number(framePosition.layer);
        #endif
    }
}

void TupGraphicsScene::drawPhotogram(int photogram, bool drawContext)
{ 
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawPhotogram()";
    #endif

    if (photogram < 0 || !gScene)
        return;

    cleanWorkSpace();
    // Painting the background
    drawSceneBackground(photogram);

    // Drawing frames from every layer
    int layersTotal = gScene->layersCount();
    TupLayer *layer;
    TupFrame *mainFrame;
    TupFrame *frame;
    for (int i=0; i < layersTotal; i++) {
         layer = gScene->layerAt(i);
         if (layer) {
             layerOnProcess = i;
             layerOnProcessOpacity = layer->getOpacity();
             int framesCount = layer->framesCount();
             zLevel = (i + 2) * ZLAYER_LIMIT;

             if (photogram < framesCount) {
                 mainFrame = layer->frameAt(photogram);
                 QString currentFrame = "";

                 if (mainFrame) {
                     if (layer->isLayerVisible()) {
                         int maximum = qMax(onionSkin.previous, onionSkin.next);
                         double opacityFactor = gOpacity / static_cast<double>(maximum);
                         double opacity = gOpacity + ((maximum - onionSkin.previous) * opacityFactor);
                         if (drawContext) {
                             // Painting previous frames
                             if (onionSkin.previous > 0 && photogram > 0) {
                                 int limit = photogram - onionSkin.previous;
                                 if (limit < 0) 
                                     limit = 0;

                                 for (int frameIndex = limit; frameIndex < photogram; frameIndex++) {
                                     frame = layer->frameAt(frameIndex);
                                     if (frame) {
                                         frameOnProcess = frameIndex;
                                         addFrame(frame, opacity, Previous);
                                         addTweeningObjects(i, frameIndex, opacity);
                                     }
                                     opacity += opacityFactor;
                                 }
                             }
                         }

                         // Painting current frame
                         frameOnProcess = photogram;
                         addFrame(mainFrame);
                         addTweeningObjects(i, photogram);
                         addSvgTweeningObjects(i, photogram);

                         if (drawContext) {
                             // Painting next frames
                             if (onionSkin.next > 0 && framesCount > photogram + 1) {
                                 opacity = gOpacity + (opacityFactor*(maximum - 1));

                                 int limit = photogram + onionSkin.next;
                                 if (limit >= framesCount)
                                     limit = framesCount - 1;

                                 for (int frameIndex = photogram+1; frameIndex <= limit; frameIndex++) {
                                     frame = layer->frameAt(frameIndex);
                                     if (frame) {
                                         frameOnProcess = frameIndex;
                                         addFrame(frame, opacity, Next);
                                         addTweeningObjects(i, frameIndex, opacity, false);
                                     }
                                     opacity -= opacityFactor;
                                 }
                             }
                         }

                         addLipSyncObjects(layer, photogram, zLevel);
                     }
                 }
             }
         }
    }

    if (gTool)
        gTool->updateScene(this);
}

void TupGraphicsScene::drawSceneBackground(int photogram)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawSceneBackground()";
    #endif

    if (!gScene) {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::drawSceneBackground() - Warning: gScene is nullptr!";
        #endif
        return;
    }

    if (background) {
        // Dynamic Bg
        if (spaceContext == TupProject::VECTOR_DYNAMIC_BG_MODE) {
            // SQA: Include option "show" raster dynamic bg as option
            // drawRasterDynamicBg();
            drawVectorDynamicBg();
            return;
        } else if (spaceContext == TupProject::VECTOR_STATIC_BG_MODE) {
            // SQA: Include option "show" raster static bg as option
            // drawRasterStaticBg();
            drawVectorStaticBg();
        } else { // Frames Mode
            QList<TupBackground::BgType> bgLayerIndex = background->layerIndexes();
            for (int i=0; i < bgLayerIndex.count(); i++) {
                #ifdef TUP_DEBUG
                    qDebug() << "TupGraphicsScene::drawSceneBackground() - Processing BG index -> " << bgLayerIndex.at(i);
                #endif

                switch(bgLayerIndex.at(i)) {
                    case TupBackground::VectorStatic:
                        drawVectorStaticBg();
                    break;
                    case TupBackground::VectorDynamic:
                        drawVectorDynamicBgOnMovement(photogram);
                    break;
                    case TupBackground::RasterStatic:
                        drawRasterStaticBg();
                    break;
                    case TupBackground::RasterDynamic:
                        drawRasterDynamicBgOnMovement(photogram);
                }
            }
        }
    }
}

void TupGraphicsScene::drawVectorStaticBg()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawVectorStaticBg()";
    #endif

    // Vector Static Bg
    if (!background->vectorStaticBgIsEmpty()) {
        TupFrame *frame = background->vectorStaticFrame();
        if (frame) {
            zLevel = ZLAYER_LIMIT;
            addFrame(frame, frame->frameOpacity());
        }
        frame = nullptr;
        delete frame;
        return;
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::drawVectorStaticBg() - Vector static bg frame is empty";
        #endif
    }
}

void TupGraphicsScene::drawVectorDynamicBg()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawVectorDynamicBg()";
    #endif

    // Vector
    if (!background->vectorDynamicBgIsEmpty()) {
        TupFrame *frame = background->vectorDynamicFrame();
        if (frame) {
            zLevel = 0;
            addFrame(frame, frame->frameOpacity());
        }
        frame = nullptr;
        delete frame;
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::drawVectorDynamicBg() - Vector dynamic bg frame is empty";
        #endif
    }
}

void TupGraphicsScene::drawVectorDynamicBgOnMovement(int photogram)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawVectorDynamicBgOnMovement() - photogram: " << photogram;
    #endif

    // Vector Dynamic Bg on movement
    if (!background->vectorDynamicBgIsEmpty()) {
        if (background->vectorRenderIsPending())
            background->renderVectorDynamicView();

        dynamicBg = new QGraphicsPixmapItem(background->vectorDynamicView(photogram));
        dynamicBg->setZValue(0);
        TupFrame *frame = background->vectorDynamicFrame();
        if (frame)
            dynamicBg->setOpacity(frame->frameOpacity());
        addItem(dynamicBg);

        dynamicBg = nullptr;
        delete dynamicBg;
        frame = nullptr;
        delete frame;
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::drawVectorDynamicBgOnMovement() - Vector dynamic bg frame is empty";
        #endif
    }
}

void TupGraphicsScene::drawRasterStaticBg()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawRasterStaticBg()";
    #endif

    // Raster Static Bg
    if (!background->rasterStaticBgIsNull()) {
        rasterStaticBg = new QGraphicsPixmapItem(background->rasterStaticBackground());
        rasterStaticBg->setZValue(0);
        addItem(rasterStaticBg);
        rasterStaticBg = nullptr;
    }
}

void TupGraphicsScene::drawRasterDynamicBg()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawRasterDynamicBg()";
    #endif

    // Raster
    if (!background->rasterDynamicBgIsNull()) {
        // Add original raster dynamic image
        rasterDynamicBg = new QGraphicsPixmapItem(background->rasterDynamicBackground());
        rasterDynamicBg->setZValue(0);
        addItem(rasterDynamicBg);
        rasterDynamicBg = nullptr;
    }
}

void TupGraphicsScene::drawRasterDynamicBgOnMovement(int photogram)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::drawRasterDynamicBgOnMovement() - photogram: " << photogram;
    #endif

    // Raster Dynamic Bg on movement
    if (!background->rasterDynamicBgIsNull()) {
        // Calculate current raster dynamic view
        if (background->rasterRenderIsPending())
            background->renderRasterDynamicView();

        rasterDynamicBg = new QGraphicsPixmapItem(background->rasterDynamicView(photogram));
        rasterDynamicBg->setZValue(0);
        addItem(rasterDynamicBg);
        rasterDynamicBg = nullptr;
    }
}

void TupGraphicsScene::addFrame(TupFrame *frame, double opacityFactor, Context mode)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::addFrame()";
    #endif
    */

    TupFrame::FrameType frameType = frame->frameType();
    QList<TupGraphicObject *> objects = frame->graphicItems(); 
    QList<TupSvgItem *> svgItems = frame->svgItems();

    int objectsCount = objects.count();
    int svgCount = svgItems.count();

    if (objectsCount == 0 && svgCount == 0)
        return;

    // Adding only native objects
    if (objectsCount > 0 && svgCount == 0) {
        foreach (TupGraphicObject *object, objects)
            processNativeObject(object, frameType, opacityFactor, mode);
        return;
    }

    // Adding only SVG objects
    if (svgCount > 0 && objectsCount == 0) {
        foreach (TupSvgItem *svg, svgItems)
            processSVGObject(svg, frameType, opacityFactor, mode);
        return;
    }

    // Adding native AND SVG objects
    TupGraphicObject *object;
    TupSvgItem *svg;
    do {
        int objectZValue = objects.at(0)->itemZValue();  
        int svgZValue = static_cast<int> (svgItems.at(0)->zValue());

        if (objectZValue < svgZValue) {
            object = objects.takeFirst();
            processNativeObject(object, frameType, opacityFactor, mode);
        } else {  
            svg = svgItems.takeFirst();
            processSVGObject(svg, frameType, opacityFactor, mode);
        }

        if (objects.isEmpty()) {
            foreach (TupSvgItem *svg, svgItems)
                processSVGObject(svg, frameType, opacityFactor, mode);
            return;
        } else {
            if (svgItems.isEmpty()) {
                foreach (TupGraphicObject *object, objects)
                    processNativeObject(object, frameType, opacityFactor, mode);
                return;
            }
        }
    } while (true);
}

void TupGraphicsScene::processNativeObject(TupGraphicObject *object, TupFrame::FrameType frameType,
                                           double opacityFactor, Context mode)
{
    if (mode != TupGraphicsScene::Current) {
        if (!object->hasTweens())
            addGraphicObject(object, frameType, opacityFactor);
    } else {
        addGraphicObject(object, frameType,  opacityFactor);
    }
}

void TupGraphicsScene::processSVGObject(TupSvgItem *svg, TupFrame::FrameType frameType,
                                        double opacityFactor, Context mode)
{
    if (mode != TupGraphicsScene::Current) {
        if (!svg->hasTweens())
            addSvgObject(svg, frameType, opacityFactor);
    } else {
        addSvgObject(svg, frameType, opacityFactor);
    }
}

void TupGraphicsScene::addGraphicObject(TupGraphicObject *object, TupFrame::FrameType frameType,
                                        double opacityFactor, bool tweenInAdvance)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::addGraphicObject()";
    #endif

    QGraphicsItem *item = object->item();
    if (item) {
        /* SQA: Code for debugging purposes 
        #ifdef TUP_DEBUG
            qWarning() << "Object XML:";
            qWarning() << object->toString();
        #endif
        */

        if (frameType == TupFrame::Regular) {
            if (framePosition.layer == layerOnProcess && framePosition.frame == frameOnProcess)
                onionSkin.accessMap.insert(item, true);
            else
                onionSkin.accessMap.insert(item, false);
        } else {
            if (spaceContext == TupProject::VECTOR_STATIC_BG_MODE || spaceContext == TupProject::VECTOR_DYNAMIC_BG_MODE)
                onionSkin.accessMap.insert(item, true);
            else
                onionSkin.accessMap.insert(item, false);
        }

        if (TupItemGroup *group = qgraphicsitem_cast<TupItemGroup *>(item))
            group->recoverChilds();

        item->setSelected(false);
        if (frameType == TupFrame::Regular)
            item->setOpacity(opacityFactor * layerOnProcessOpacity);
        else
            item->setOpacity(opacityFactor);

        if (!object->hasTweens() || !tweenInAdvance) {
            item->setZValue(zLevel);
            zLevel++;
        }

        addItem(item);
    }
}

void TupGraphicsScene::addSvgObject(TupSvgItem *svgItem, TupFrame::FrameType frameType,
                                    double opacityFactor, bool tweenInAdvance)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::addSvgObject()";
    #endif
    */

    if (svgItem) {
        svgItem->setSelected(false);
        if (frameType == TupFrame::Regular) {
            if (framePosition.layer == layerOnProcess && framePosition.frame == frameOnProcess)
                onionSkin.accessMap.insert(svgItem, true);
            else
                onionSkin.accessMap.insert(svgItem, false);
        } else {
            if (spaceContext == TupProject::VECTOR_STATIC_BG_MODE || spaceContext == TupProject::VECTOR_DYNAMIC_BG_MODE)
                onionSkin.accessMap.insert(svgItem, true);
            else
                onionSkin.accessMap.insert(svgItem, false);
        }

        TupLayer *layer = gScene->layerAt(framePosition.layer);
        if (layer) {
            TupFrame *frame = layer->frameAt(framePosition.frame);
            if (frame) {
                if (frameType == TupFrame::Regular)
                    svgItem->setOpacity(opacityFactor * layerOnProcessOpacity);
                else
                    svgItem->setOpacity(opacityFactor);

                // SQA: Experimental code related to interactive features
                // if (svgItem->symbolName().compare("dollar.svg")==0)
                //     connect(svgItem, SIGNAL(enabledChanged()), this, SIGNAL(showInfoWidget()));

                if (!svgItem->hasTweens() || !tweenInAdvance) {
                    svgItem->setZValue(zLevel);
                    zLevel++;
                }

                // SQA: Pending to implement SVG group support

                addItem(svgItem);
            } else {
                #ifdef TUP_DEBUG
                    qDebug() << "TupGraphicsScene::addSvgObject() - Error: Frame #"
                                + QString::number(framePosition.frame) + " NO available!";
                #endif
            }
        } else {
            #ifdef TUP_DEBUG
                qDebug() << "TupGraphicsScene::addSvgObject() - Error: Layer #"
                            + QString::number(framePosition.layer) + " NO available!";
            #endif
        }
    } else {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::addSvgObject() - Error: No SVG item!";
        #endif
    } 
} 

void TupGraphicsScene::addTweeningObjects(int layerIndex, int photogram, double opacity, bool onProcess)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::addTweeningObjects()";
    #endif

    QList<TupGraphicObject *> tweenList = gScene->getTweeningGraphicObjects(layerIndex);
    int total = tweenList.count();

    #ifdef TUP_DEBUG
        qWarning() << "Tween list size: " << QString::number(total);
    #endif

    TupGraphicObject *object;
    TupTweenerStep *stepItem;
    for (int i=0; i < total; i++) {
         object = tweenList.at(i);
         int origin = object->frameIndex();

         QList<TupItemTweener *> list = object->tweensList();
         foreach(TupItemTweener *tween, list) {
             int adjustX = static_cast<int> (object->item()->boundingRect().width())/2;
             int adjustY = static_cast<int> (object->item()->boundingRect().height())/2;

             if (origin == photogram) {
                 #ifdef TUP_DEBUG
                     qWarning() << "Adding FIRST tween transformation - photogram -> " << QString::number(photogram);
                 #endif

                 stepItem = tween->stepAt(0);
                 if (stepItem->has(TupTweenerStep::Position)) {
                     QPointF point = QPoint(-adjustX, -adjustY);
                     object->setLastTweenPos(stepItem->getPosition() + point);
                     object->item()->setPos(tween->transformOriginPoint());
                 } else if (stepItem->has(TupTweenerStep::Rotation)) {
                     double angle = stepItem->getRotation();
                     object->item()->setTransformOriginPoint(tween->transformOriginPoint());
                     object->item()->setRotation(angle);
                 } else if (stepItem->has(TupTweenerStep::Scale)) {
                     QPointF point = tween->transformOriginPoint();
                     double scaleX = tween->initXScaleValue();
                     double scaleY = tween->initYScaleValue();
                     QTransform transform;
                     transform.translate(point.x(), point.y());
                     transform.scale(scaleX, scaleY);
                     transform.translate(-point.x(), -point.y());

                     object->item()->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Shear)) {
                     QTransform transform;
                     transform.shear(0, 0);
                     object->item()->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Coloring)) {
                     if (tween->tweenColorFillType() == TupItemTweener::Line || tween->tweenColorFillType() == TupItemTweener::FillAll) {
                         QColor itemColor = stepItem->getColor();
                         if (TupPathItem *path = qgraphicsitem_cast<TupPathItem *>(object->item())) {
                             QPen pen = path->pen();
                             pen.setColor(itemColor);
                             path->setPen(pen);
                         } else if (TupEllipseItem *ellipse = qgraphicsitem_cast<TupEllipseItem *>(object->item())) {
                             QPen pen = ellipse->pen();
                             pen.setColor(itemColor);
                             ellipse->setPen(pen);
                         } else if (TupLineItem *line = qgraphicsitem_cast<TupLineItem *>(object->item())) {
                             QPen pen = line->pen();
                             pen.setColor(itemColor);
                             line->setPen(pen);
                         } else if (TupRectItem *rect = qgraphicsitem_cast<TupRectItem *>(object->item())) {
                             QPen pen = rect->pen();
                             pen.setColor(itemColor);
                             rect->setPen(pen);
                         }
                     }

                     if (tween->tweenColorFillType() == TupItemTweener::Internal || tween->tweenColorFillType() == TupItemTweener::FillAll) {
                         QColor itemColor = stepItem->getColor();
                         if (TupPathItem *path = qgraphicsitem_cast<TupPathItem *>(object->item())) {
                             QBrush brush = path->brush();
                             brush.setColor(itemColor);
                             path->setBrush(brush);
                         } else if (TupEllipseItem *ellipse = qgraphicsitem_cast<TupEllipseItem *>(object->item())) {
                             QBrush brush = ellipse->brush();
                             brush.setColor(itemColor);
                             ellipse->setBrush(brush);
                         } else if (TupRectItem *rect = qgraphicsitem_cast<TupRectItem *>(object->item())) {
                             QBrush brush = rect->brush();
                             brush.setColor(itemColor);
                             rect->setBrush(brush);
                         }
                     }
                 } else if (stepItem->has(TupTweenerStep::Opacity)) {
                     object->item()->setOpacity(stepItem->getOpacity());
                 }

                 if (!onProcess && opacity < 1) {
                     // Including object into the scene
                     addGraphicObject(object, TupFrame::Regular, 1.0, true);
                     object->item()->setOpacity(opacity);
                 }
             } else if ((origin < photogram) && (photogram < origin + tween->getFrames())) {
                 if (!onProcess)
                     return;

                 #ifdef TUP_DEBUG
                     qWarning() << "Adding tween transformation - photogram -> " << QString::number(photogram);
                 #endif
                 int step = photogram - origin;
                 stepItem = tween->stepAt(step);
                 if (stepItem->has(TupTweenerStep::Position)) {
                     qreal dx = stepItem->getPosition().x() - (object->lastTweenPos().x() + adjustX);
                     qreal dy = stepItem->getPosition().y() - (object->lastTweenPos().y() + adjustY);
                     object->item()->moveBy(dx, dy);
                     QPointF point = QPoint(-adjustX, -adjustY);
                     object->setLastTweenPos(stepItem->getPosition() + point);
                 } else if (stepItem->has(TupTweenerStep::Rotation)) {
                     double angle = stepItem->getRotation();
                     object->item()->setRotation(angle);
                 } else if (stepItem->has(TupTweenerStep::Scale)) {
                     QPointF point = tween->transformOriginPoint();

                     double scaleX = stepItem->horizontalScale();
                     double scaleY = stepItem->verticalScale();
                     QTransform transform;
                     transform.translate(point.x(), point.y());
                     transform.scale(scaleX, scaleY);
                     transform.translate(-point.x(), -point.y());

                     object->item()->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Shear)) {
                     QPointF point = tween->transformOriginPoint();

                     double shearX = stepItem->horizontalShear();
                     double shearY = stepItem->verticalShear();
                     QTransform transform;
                     transform.translate(point.x(), point.y());
                     transform.shear(shearX, shearY);
                     transform.translate(-point.x(), -point.y());

                     object->item()->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Coloring)) {
                     if (tween->tweenColorFillType() == TupItemTweener::Line || tween->tweenColorFillType() == TupItemTweener::FillAll) {
                         QColor itemColor = stepItem->getColor();
                         if (TupPathItem *path = qgraphicsitem_cast<TupPathItem *>(object->item())) {
                             QPen pen = path->pen();
                             pen.setColor(itemColor);
                             path->setPen(pen);
                         } else if (TupEllipseItem *ellipse = qgraphicsitem_cast<TupEllipseItem *>(object->item())) {
                             QPen pen = ellipse->pen();
                             pen.setColor(itemColor);
                             ellipse->setPen(pen);
                         } else if (TupLineItem *line = qgraphicsitem_cast<TupLineItem *>(object->item())) {
                             QPen pen = line->pen();
                             pen.setColor(itemColor);
                             line->setPen(pen);
                         } else if (TupRectItem *rect = qgraphicsitem_cast<TupRectItem *>(object->item())) {
                             QPen pen = rect->pen();
                             pen.setColor(itemColor);
                             rect->setPen(pen);
                         }
                     }

                     if (tween->tweenColorFillType() == TupItemTweener::Internal || tween->tweenColorFillType() == TupItemTweener::FillAll) {
                         QColor itemColor = stepItem->getColor();
                         if (TupPathItem *path = qgraphicsitem_cast<TupPathItem *>(object->item())) {
                             QBrush brush = path->brush();
                             brush.setColor(itemColor);
                             path->setBrush(brush);
                         } else if (TupEllipseItem *ellipse = qgraphicsitem_cast<TupEllipseItem *>(object->item())) {
                             QBrush brush = ellipse->brush();
                             brush.setColor(itemColor);
                             ellipse->setBrush(brush);
                         } else if (TupRectItem *rect = qgraphicsitem_cast<TupRectItem *>(object->item())) {
                             QBrush brush = rect->brush();
                             brush.setColor(itemColor);
                             rect->setBrush(brush);
                         }
                     }
                 }
                 // Including object into the scene
                 addGraphicObject(object, TupFrame::Regular, 1.0, true);

                 if (stepItem->has(TupTweenerStep::Opacity))
                     object->item()->setOpacity(stepItem->getOpacity());

                 if (opacity < 1) {
                     object->item()->setOpacity(opacity);
                     return;
                 }
             }

             QString tip = object->item()->toolTip();
             if (tip.length() == 0) {
                 object->item()->setToolTip("Tweens: " + tween->tweenTypeToString());
             } else if (!tip.contains(tween->tweenTypeToString())) {
                 tip += "," + tween->tweenTypeToString();
                 object->item()->setToolTip(tip);
             }
         }
    }
}

void TupGraphicsScene::addSvgTweeningObjects(int indexLayer, int photogram)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::addSvgTweeningObjects()";
    #endif

    QList<TupSvgItem *> svgList = gScene->getTweeningSvgObjects(indexLayer);
    TupSvgItem *object;
    TupTweenerStep *stepItem;
    for (int i=0; i < svgList.count(); i++) {
         object = svgList.at(i);
         int origin = object->frameIndex();

         QList<TupItemTweener *> list = object->tweensList();
         foreach(TupItemTweener *tween, list) {
             int adjustX = static_cast<int> (object->boundingRect().width()/2);
             int adjustY = static_cast<int> (object->boundingRect().height()/2);

             if (origin == photogram) {
                 #ifdef TUP_DEBUG
                     qDebug() << "Adding FIRST SVG tween transformation - photogram -> " + QString::number(photogram);
                 #endif

                 stepItem = tween->stepAt(0);

                 if (stepItem->has(TupTweenerStep::Position)) {
                     object->setPos(tween->transformOriginPoint());
                     QPointF offset = QPoint(-adjustX, -adjustY);
                     object->setLastTweenPos(stepItem->getPosition() + offset);
                 } else if (stepItem->has(TupTweenerStep::Rotation)) {
                     double angle = stepItem->getRotation();
                     object->setTransformOriginPoint(tween->transformOriginPoint());
                     object->setRotation(angle);
                 } else if (stepItem->has(TupTweenerStep::Scale)) {
                     QPointF point = tween->transformOriginPoint();
                     double scaleX = tween->initXScaleValue();
                     double scaleY = tween->initYScaleValue();
                     QTransform transform;
                     transform.translate(point.x(), point.y());
                     transform.scale(scaleX, scaleY);
                     transform.translate(-point.x(), -point.y());

                     object->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Shear)) {
                     QTransform transform;
                     transform.shear(0, 0);
                     object->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Opacity)) {
                     object->setOpacity(stepItem->getOpacity());
                 }
             } else if ((origin < photogram) && (photogram < origin + tween->getFrames())) {
                 #ifdef TUP_DEBUG
                     qDebug() << "Adding SVG tween transformation - photogram -> " + QString::number(photogram);
                 #endif

                 int step = photogram - origin;
                 stepItem = tween->stepAt(step);

                 if (stepItem->has(TupTweenerStep::Position)) {
                     qreal dx = stepItem->getPosition().x() - (object->lastTweenPos().x() + adjustX);
                     qreal dy = stepItem->getPosition().y() - (object->lastTweenPos().y() + adjustY);
                     object->moveBy(dx, dy);
                     QPointF offset = QPoint(-adjustX, -adjustY);
                     object->setLastTweenPos(stepItem->getPosition() + offset);
                 } else if (stepItem->has(TupTweenerStep::Rotation)) {
                     double angle = stepItem->getRotation();
                     object->setRotation(angle);
                 } else if (stepItem->has(TupTweenerStep::Scale)) {
                     QPointF point = tween->transformOriginPoint();

                     double scaleX = stepItem->horizontalScale();
                     double scaleY = stepItem->verticalScale();
                     QTransform transform;
                     transform.translate(point.x(), point.y());
                     transform.scale(scaleX, scaleY);
                     transform.translate(-point.x(), -point.y());

                     object->setTransform(transform);
                 } else if (stepItem->has(TupTweenerStep::Shear)) {
                     QPointF point = tween->transformOriginPoint();

                     double shearX = stepItem->horizontalShear();
                     double shearY = stepItem->verticalShear();
                     QTransform transform;
                     transform.translate(point.x(), point.y());
                     transform.shear(shearX, shearY);
                     transform.translate(-point.x(), -point.y());

                     object->setTransform(transform);
                 }

                 addSvgObject(object, TupFrame::Regular, 1.0, true);

                 if (stepItem->has(TupTweenerStep::Opacity))
                     object->setOpacity(stepItem->getOpacity());
             }

             QString tip = object->toolTip();
             if (tip.length() == 0) {
                 object->setToolTip("Tweens: " + tween->tweenTypeToString());
             } else if (!tip.contains(tween->tweenTypeToString())) {
                 tip += "," + tween->tweenTypeToString();
                 object->setToolTip(tip);
             }
         }
    }
}

void TupGraphicsScene::addLipSyncObjects(TupLayer *layer, int photogram, int zValue)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::addLipSyncObjects()";
    #endif

    if (layer->lipSyncCount() > 0) {
        Mouths mouths = layer->getLipSyncList();
        int total = mouths.count();

        TupLipSync *lipSync;
        TupLibraryFolder *folder;
        TupVoice *voice;
        TupPhoneme *phoneme;
        TupLibraryObject *mouthImg;
        TupGraphicLibraryItem *item;
        for (int i=0; i<total; i++) {
             lipSync = mouths.at(i);
             int initFrame = lipSync->getInitFrame();

             if ((photogram >= initFrame) && (photogram <= initFrame + lipSync->getFramesCount())) {
                 QString name = lipSync->getLipSyncName();
                 folder = library->getFolder(name);
                 if (folder) {
                     QList<TupVoice *> voices = lipSync->getVoices();
                     int voicesTotal = voices.count();
                     for(int j=0; j<voicesTotal; j++) {
                         voice = voices.at(j);
                         int index = photogram - initFrame; 
                         if (voice->contains(index)) {
                             // Adding phoneme image
                             phoneme = voice->getPhonemeAt(index);
                             if (phoneme) {
                                 QString imgName = phoneme->value() + lipSync->getPicExtension();
                                 mouthImg = folder->getObject(imgName);
                                 if (mouthImg) {
                                     item = new TupGraphicLibraryItem(mouthImg);
                                     if (item) {
                                         QPointF pos = phoneme->position();
                                         int wDelta = static_cast<int> (item->boundingRect().width()/2);
                                         int hDelta = static_cast<int> (item->boundingRect().height()/2);
                                         item->setPos(pos.x()-wDelta, pos.y()-hDelta);
                                         item->setToolTip(tr("lipsync:") + name + ":" + QString::number(j));
                                         item->setZValue(zValue);
                                         addItem(item);
                                     }
                                 } else {
                                     #ifdef TUP_DEBUG
                                         qDebug() << "TupGraphicsScene::addLipSyncObjects() - Warning: Can't find phoneme image -> "
                                                     + imgName;
                                     #endif
                                 } 
                             } else {
                                 #ifdef TUP_DEBUG
                                     qDebug() << "TupGraphicsScene::addLipSyncObjects() - Warning: No lipsync phoneme at frame "
                                                 + QString::number(photogram) + " - index: " + QString::number(index);
                                 #endif

                                 // Adding rest phoneme to cover empty frame
                                 QString imgName = "rest" + lipSync->getPicExtension();
                                 mouthImg = folder->getObject(imgName);
                                 if (mouthImg) {
                                     item = new TupGraphicLibraryItem(mouthImg);
                                     if (item) {
                                         QPointF pos = voice->mouthPos();
                                         int wDelta = static_cast<int> (item->boundingRect().width()/2);
                                         int hDelta = static_cast<int> (item->boundingRect().height()/2);
                                         item->setPos(pos.x()-wDelta, pos.y()-hDelta);
                                         item->setToolTip(tr("lipsync:") + name + ":" + QString::number(j));
                                         item->setZValue(zValue);
                                         addItem(item);
                                     }
                                 } else {
                                     #ifdef TUP_DEBUG
                                         qDebug() << "TupGraphicsScene::addLipSyncObjects() - Warning: Can't find phoneme image -> "
                                                     + imgName;
                                     #endif
                                 }
                             }
                         } else {
                             #ifdef TUP_DEBUG
                                 qDebug() << "TupGraphicsScene::addLipSyncObjects() - No lipsync phoneme in voice at position: "
                                             + QString::number(j) + " - looking for index: " + QString::number(index);
                             #endif
                         }
                     }
                 } else {
                     #ifdef TUP_DEBUG
                         qDebug() << "TupGraphicsScene::addLipSyncObjects() - Folder with lipsync mouths is not available -> "
                                     + name;
                     #endif
                 } 
             }
        }
    }
}

void TupGraphicsScene::cleanWorkSpace()
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::cleanWorkSpace()";
    #endif
    */

    if (dynamicBg) {
        dynamicBg = nullptr;
        delete dynamicBg;
    }

    onionSkin.accessMap.clear();

    foreach (QGraphicsItem *item, items()) {
        if (item->scene() == this)
            removeItem(item);
    }

    foreach (TupLineGuide *line, lines)
        addItem(line);
}

int TupGraphicsScene::currentFrameIndex() const
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::currentFrameIndex()";
    #endif
    */

    return framePosition.frame;
}

int TupGraphicsScene::currentLayerIndex() const
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::currentLayerIndex()";
    #endif
    */

    return framePosition.layer;
}

int TupGraphicsScene::currentSceneIndex() const
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::currentSceneIndex()";
    #endif
    */

    if (!gScene) {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::currentSceneIndex() - Error: Scene index is -1";
        #endif
        return -1;
    }

    return gScene->objectIndex();
}

void TupGraphicsScene::setNextOnionSkinCount(int n)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setNextOnionSkinCount()";
    #endif
    */

    onionSkin.next = n;

    if (spaceContext == TupProject::FRAMES_MODE)
        drawCurrentPhotogram();
}

void TupGraphicsScene::setPreviousOnionSkinCount(int n)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setPreviousOnionSkinCount()";
    #endif
    */

    onionSkin.previous = n;

    if (spaceContext == TupProject::FRAMES_MODE)
        drawCurrentPhotogram();
}

TupFrame *TupGraphicsScene::currentFrame()
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::currentFrame()";
    #endif
    */

    if (gScene) {
        if (gScene->layersCount() > 0) {
            if (framePosition.layer < gScene->layersCount()) {
                TupLayer *layer = gScene->layerAt(framePosition.layer);
                // Q_CHECK_PTR(layer);
                if (layer) {
                    if (!layer->getFrames().isEmpty())
                        return layer->frameAt(framePosition.frame);
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "TupGraphicsScene::currentFrame - No layer available at -> "
                                    + QString::number(framePosition.frame);
                    #endif
                }
            } else {
                TupLayer *layer = gScene->layerAt(gScene->layersCount() - 1);
                if (layer) {
                    if (!layer->getFrames().isEmpty())
                        return layer->frameAt(framePosition.frame);
                }
            }

        }

    }

    return nullptr;
}

void TupGraphicsScene::setCurrentScene(TupScene *pScene)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setCurrentScene()";
    #endif

    if (pScene) {
        setCurrentFrame(0, 0);
        if (gTool)
            gTool->aboutToChangeScene(this);
        qDeleteAll(lines);
        lines.clear();

        cleanWorkSpace();
        gScene = pScene;
        background = gScene->sceneBackground();

        if (spaceContext == TupProject::FRAMES_MODE)
            drawCurrentPhotogram();
        else
            drawSceneBackground(framePosition.frame);
    }
}

TupScene *TupGraphicsScene::currentScene() const
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::scene()";
    #endif
    */

    if (gScene)
        return gScene;
    else
        return nullptr;
}

void TupGraphicsScene::setTool(TupToolPlugin *plugin)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setTool()";
    #endif

    if (spaceContext == TupProject::FRAMES_MODE) {
        drawCurrentPhotogram();
    } else {
        cleanWorkSpace();
        drawSceneBackground(framePosition.frame);
    }

    if (gTool)
        gTool->aboutToChangeTool();

    gTool = plugin;
    gTool->init(this);
}

TupToolPlugin *TupGraphicsScene::currentTool() const
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::currentTool()";
    #endif
    */

    return gTool;
}

void TupGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::mousePressEvent()";
    #endif
    */

    QGraphicsScene::mousePressEvent(event);
    inputInformation->updateFromMouseEvent(event);
    isDrawing = false;

    // This condition locks all the tools while workspace is rotated 
    if ((event->buttons() != Qt::LeftButton) || (event->modifiers () != (Qt::ShiftModifier | Qt::ControlModifier))) {
        if (gTool) {
            if (gTool->toolType() == TupToolPlugin::Brush && event->isAccepted())
                return;

            if (gTool->toolType() == TupToolPlugin::Tweener && event->isAccepted()) {
                if (gTool->currentEditMode() == TupToolPlugin::Properties)
                    return;
            } 

            // If there's no frame... the tool is disabled 
            if (currentFrame()) {
                if (event->buttons() == Qt::LeftButton) {
                    gTool->begin();
                    isDrawing = true;
                    gTool->press(inputInformation, brushManager, this);
                } 
            } 
        }
    }
}

void TupGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::mouseMoveEvent()";
    #endif
    */

    QGraphicsScene::mouseMoveEvent(event);
    mouseMoved(event);

    if (gTool) {
        QString tool = gTool->name();
        if (tool.compare(tr("Line")) == 0 || tool.compare(tr("PolyLine")) == 0)
            gTool->updatePos(event->scenePos());
    }
}

void TupGraphicsScene::mouseMoved(QGraphicsSceneMouseEvent *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::mouseMoved()";
    #endif
    */

    inputInformation->updateFromMouseEvent(event);

    if (gTool && isDrawing)
        gTool->move(inputInformation, brushManager,  this);
}

void TupGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::mouseReleaseEvent()";
    #endif
    */

    // SQA: Temporal solution for cases when there's no current frame defined
    if (!currentFrame())
        return;

    QGraphicsScene::mouseReleaseEvent(event);
    mouseReleased(event);
}

void TupGraphicsScene::mouseReleased(QGraphicsSceneMouseEvent *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::mouseReleased()";
    #endif
    */

    if (gTool) {
        if (gTool->toolType() == TupToolInterface::Brush) {
            if (event->button() == Qt::RightButton) 
                return;
        }
    }

    if (currentFrame()) {
        if (currentFrame()->isFrameLocked()) {
            #ifdef TUP_DEBUG
                qDebug() << "TupGraphicsScene::mouseReleased() - Frame is locked!";
            #endif
            return;
        }
    }

    inputInformation->updateFromMouseEvent(event);

    if (isDrawing) {
        if (gTool) {
            gTool->release(inputInformation, brushManager, this);
            gTool->end();
        }
    } 

    isDrawing = false;
}

void TupGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::mouseDoubleClickEvent()";
    #endif
    */

    QGraphicsScene::mouseDoubleClickEvent(event);
    inputInformation->updateFromMouseEvent(event);

    if (gTool)
        gTool->doubleClick(inputInformation, this);
}

void TupGraphicsScene::keyPressEvent(QKeyEvent *event)
{
    if (gTool) {
        gTool->keyPressEvent(event);
        
        if (event->isAccepted())
            return;
    }
    
    QGraphicsScene::keyPressEvent(event);
}

void TupGraphicsScene::keyReleaseEvent(QKeyEvent *event)
{
    if (gTool) {
        gTool->keyReleaseEvent(event);

        if (event->isAccepted())
            return;
    }
   
    QGraphicsScene::keyReleaseEvent(event);
}

/*
// SQA: Check this code, not sure whether it does something or it's handy :S
void TupGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent * event)
{
    if (event->mimeData()->hasFormat("tupi-ruler"))
        event->acceptProposedAction();

    TupLineGuide *line = 0;
    if (event->mimeData()->data("tupi-ruler") == "verticalLine") {
        line  = new TupLineGuide(Qt::Vertical, this);
        line->setPos(event->scenePos());
    } else {
        line = new TupLineGuide(Qt::Horizontal, this);
        line->setPos(event->scenePos());
    }

    if (line)
        lines << line;
}

// TODO: Check this code, not sure whether it does something or it is helping :S

void  TupGraphicsScene::dragLeaveEvent(QGraphicsSceneDragDropEvent * event)
{
    Q_UNUSED(event);

    removeItem(lines.last());
    delete lines.takeLast();
}

// TODO: Check this code, not sure whether it does something or it is helping :S

void TupGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent * event)
{
    if (!lines.isEmpty())
        lines.last()->setPos(event->scenePos());
}

// TODO: Check this code, not sure whether it does something or it is helping :S

void TupGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent * event)
{
    Q_UNUSED(event);

    if (gTool) {
        if (gTool->toolType() == TupToolPlugin::ObjectsTool) {
            lines.last()->setEnabledSyncCursor(false);
            lines.last()->setFlag(QGraphicsItem::ItemIsMovable, true);
        }
    }
}
*/

bool TupGraphicsScene::event(QEvent *event)
{
    return QGraphicsScene::event(event);
}

void TupGraphicsScene::sceneResponse(TupSceneResponse *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::sceneResponse()";
    #endif
    */

    if (gTool)
        gTool->sceneResponse(event);
}

void TupGraphicsScene::layerResponse(TupLayerResponse *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupGraphicsScene::layerResponse()]";
    #endif
    */

    if (gTool)
        gTool->layerResponse(event);
}

void TupGraphicsScene::frameResponse(TupFrameResponse *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupGraphicsScene::frameResponse()]";
    #endif
    */

    if (gTool)
        gTool->frameResponse(event);
}

void TupGraphicsScene::itemResponse(TupItemResponse *event)
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "[TupGraphicsScene::itemResponse()]";
    #endif
    */
   
    if (gTool)
        gTool->itemResponse(event);
}

bool TupGraphicsScene::userIsDrawing() const
{
    return isDrawing;
}

TupBrushManager *TupGraphicsScene::getBrushManager() const
{
    return brushManager;
}

void TupGraphicsScene::setSelectionRange()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setSelectionRange()";
    #endif

    if (onionSkin.accessMap.empty() || gTool->toolType() == TupToolInterface::Tweener)
        return;

    QHash<QGraphicsItem *, bool>::iterator it = onionSkin.accessMap.begin();
    QString tool = gTool->name();
    if (tool.compare(tr("Object Selection")) == 0 || tool.compare(tr("Nodes Selection")) == 0) {
        while (it != onionSkin.accessMap.end()) {
            if (!it.value() || it.key()->toolTip().length() > 0) {
                it.key()->setAcceptedMouseButtons(Qt::NoButton);
                it.key()->setFlag(QGraphicsItem::ItemIsSelectable, false);
                it.key()->setFlag(QGraphicsItem::ItemIsMovable, false);
            } else {
                it.key()->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MidButton | Qt::XButton1
                                                  | Qt::XButton2);
                if (tool.compare(tr("Object Selection")) == 0) {
                    it.key()->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
                } else {
                    it.key()->setFlag(QGraphicsItem::ItemIsSelectable, true);
                    it.key()->setFlag(QGraphicsItem::ItemIsMovable, false);
                }
            }
            ++it;
        }
    } else {
        while (it != onionSkin.accessMap.end()) {
            it.key()->setAcceptedMouseButtons(Qt::NoButton);
            it.key()->setFlag(QGraphicsItem::ItemIsSelectable, false);
            it.key()->setFlag(QGraphicsItem::ItemIsMovable, false);
            ++it;
        }
    }
}

void TupGraphicsScene::enableItemsForSelection()
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::enableItemsForSelection()";
    #endif

    QHash<QGraphicsItem *, bool>::iterator it = onionSkin.accessMap.begin();
    while (it != onionSkin.accessMap.end()) {
        // if (it.value() && it.key()->toolTip().length() == 0)
        if (it.value())
            it.key()->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
        ++it;
    }
}

void TupGraphicsScene::includeObject(QGraphicsItem *object, bool isPolyLine) // SQA: Bool parameter is a hack for the Polyline tool. Must be fixed. 
{
    /*
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::includeObject()";
    #endif
    */

    if (!object) {
        #ifdef TUP_DEBUG
            qDebug() << "TupGraphicsScene::includeObject() - Fatal Error: Graphic item is nullptr!";
            qDebug() << "Space Context: " << spaceContext;
        #endif

        return;
    }

    if (spaceContext == TupProject::FRAMES_MODE) {
        TupLayer *layer = gScene->layerAt(framePosition.layer);
        if (layer) {
            TupFrame *frame = layer->frameAt(framePosition.frame);
            if (frame) {
                // SQA: The constant 100 assumes than 100 items have been created per frame
                int zValue = (gScene->framesCount()*100) + frame->getTopZLevel();
                if (isPolyLine) // SQA: Polyline hack
                    zValue--;

                qreal opacity = layer->getOpacity(); 
                if (opacity >= 0 && opacity <= 1) {
                    object->setOpacity(opacity);
                } else {
                    #ifdef TUP_DEBUG
                        qDebug() << "TupGraphicsScene::includeObject() - Fatal Error: Opacity value is invalid -> "
                                    + QString::number(opacity);
                    #endif
                }

                object->setZValue(zValue);
                addItem(object);
                zLevel++;
            }
        }
    } else {
        // TupBackground *bg = gScene->sceneBackground();
        if (background) {
            TupFrame *frame = new TupFrame;
            if (spaceContext == TupProject::VECTOR_STATIC_BG_MODE) {
                frame = background->vectorStaticFrame();
            } else if (spaceContext == TupProject::VECTOR_DYNAMIC_BG_MODE) {
                frame = background->vectorDynamicFrame();
            }

            if (frame) {
                int zValue = frame->getTopZLevel();
                object->setZValue(zValue);
                addItem(object);
            }
        }
    }
}

void TupGraphicsScene::removeScene()
{
    cleanWorkSpace();
    gScene = nullptr;
}

TupProject::Mode TupGraphicsScene::getSpaceContext()
{
    return spaceContext;
}

void TupGraphicsScene::setSpaceMode(TupProject::Mode mode)
{
    spaceContext = mode;
}

void TupGraphicsScene::setOnionFactor(double opacity)
{
    gOpacity = opacity;

    if (spaceContext == TupProject::FRAMES_MODE)
        drawCurrentPhotogram();
}

double TupGraphicsScene::getOpacity()
{
    return gOpacity;
}

int TupGraphicsScene::getFramesCount()
{
    TupLayer *layer = gScene->layerAt(framePosition.layer);
    if (layer)
        return layer->framesCount();
    else
        return -1;
}

void TupGraphicsScene::setLibrary(TupLibrary *assets)
{
    #ifdef TUP_DEBUG
        qDebug() << "TupGraphicsScene::setLibrary()";
    #endif

    library = assets;
}

void TupGraphicsScene::resetCurrentTool() 
{
    gTool->init(this);
}

TupInputDeviceInformation * TupGraphicsScene::inputDeviceInformation()
{
    return inputInformation;
}

void TupGraphicsScene::updateLoadingFlag(bool flag)
{
    loadingProject = flag;
}
