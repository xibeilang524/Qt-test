import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick 2.12
 import QtQuick.Controls 2.12
Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")




     Menu {
         id: menu

        visible: true
         Action { text: qsTr("Tool Bar"); checkable: true }
         Action { text: qsTr("Side Bar"); checkable: true; checked: true }
         Action { text: qsTr("Status Bar"); checkable: true; checked: true }

         MenuSeparator {
             contentItem: Rectangle {
                 implicitWidth: 200
                 implicitHeight: 1
                 color: "#21be2b"
             }
         }

         Menu {
             title: qsTr("Advanced")
             // ...
         }

         topPadding: 2
         bottomPadding: 2

         delegate: MenuItem {
             id: menuItem
             implicitWidth: 200
             implicitHeight: 40

             arrow: Canvas {
                 x: parent.width - width
                 implicitWidth: 40
                 implicitHeight: 40
                 visible: menuItem.subMenu
                 onPaint: {
                     var ctx = getContext("2d")
                     ctx.fillStyle = menuItem.highlighted ? "#ffffff" : "#21be2b"
                     ctx.moveTo(15, 15)
                     ctx.lineTo(width - 15, height / 2)
                     ctx.lineTo(15, height - 15)
                     ctx.closePath()
                     ctx.fill()
                 }
             }

             indicator: Item {
                 implicitWidth: 40
                 implicitHeight: 40
                 Rectangle {
                     width: 26
                     height: 26
                     anchors.centerIn: parent
                     visible: menuItem.checkable
                     border.color: "#21be2b"
                     radius: 3
                     Rectangle {
                         width: 14
                         height: 14
                         anchors.centerIn: parent
                         visible: menuItem.checked
                         color: "#21be2b"
                         radius: 2
                     }
                 }
             }

             contentItem: Text {
                 leftPadding: menuItem.indicator.width
                 rightPadding: menuItem.arrow.width
                 text: menuItem.text
                 font: menuItem.font
                 opacity: enabled ? 1.0 : 0.3
                 color: menuItem.highlighted ? "#ffffff" : "#21be2b"
                 horizontalAlignment: Text.AlignLeft
                 verticalAlignment: Text.AlignVCenter
                 elide: Text.ElideRight
             }

             background: Rectangle {
                 implicitWidth: 200
                 implicitHeight: 40
                 opacity: enabled ? 1 : 0.3
                 color: menuItem.highlighted ? "#21be2b" : "transparent"
             }
         }

         background: Rectangle {
             implicitWidth: 200
             implicitHeight: 40
             color: "#ffffff"
             border.color: "#21be2b"
             radius: 2
         }
    }
}
