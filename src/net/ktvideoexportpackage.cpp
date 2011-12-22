/***************************************************************************
 *   Project TUPI: Magia 2D                                                *
 *   Project Contact: info@maefloresta.com                                 *
 *   Project Website: http://www.maefloresta.com                           *
 *   Project Leader: Gustav Gonzalez <info@maefloresta.com>                *
 *                                                                         *
 *   Developers:                                                           *
 *   2010:                                                                 *
 *    Gustavo Gonzalez / xtingray                                          *
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
 *   the Free Software Foundation; either version 3 of the License, or     *
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

#include "ktvideoexportpackage.h"

// <project_video version="0">
//     <video scenes="0, 1, 2, N" />
//         <title>Video Title</title>
//         <description>Video Description</description>          
//     </video>
// </project_video>

KTVideoExportPackage::KTVideoExportPackage(const QString &title, const QString &description, const QList<int> sceneIndexes): QDomDocument()
{
    QDomElement root = createElement("project_video");
    root.setAttribute("version", "0");
    appendChild(root);
    
    QDomElement video = createElement("video");

    QString indexes = "";
    for (int i=0; i < sceneIndexes.size(); i++)
         indexes += QString::number(sceneIndexes.at(i)) + ","; 

    indexes.remove(indexes.length() - 1, 1);

    video.setAttribute("scenes", indexes);

    QDomText titleDom = createTextNode(title);
    QDomText descDom = createTextNode(description);

    video.appendChild(createElement("title")).appendChild(titleDom);
    video.appendChild(createElement("description")).appendChild(descDom);
    
    root.appendChild(video);
}

KTVideoExportPackage::~KTVideoExportPackage()
{
}