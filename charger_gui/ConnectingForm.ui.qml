import QtQuick 2.4
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.4
import QtQuick.Extras 1.4

Item {
    id: content
    
    width: 400
    height: 250
    
    property bool connected: false
    property int itemWidth: 50
    property alias cancelButton: actionButton
    property alias connectionMessage: labelConnectionState.text
    property alias modelView: modelView
    
    states: [
        State {
            name: "ConnectingState"
            when: !connected
            
            PropertyChanges {
                target: labelConnectionState
                text: qsTr("Connecting...")
            }
            
            PropertyChanges {
                target: actionButton
                visible: true
            }
            
            PropertyChanges {
                target: content
                height: 300
            }
        },
        
        State {
            name: "ConnectedState"
            when: connected
            
            PropertyChanges {
                target: labelConnectionState
                text: qsTr("Choose Device...")
            }
            
            PropertyChanges {
                target: actionButton
                text: "Continue"
                isDefault: true
                visible: true
            }
            
            PropertyChanges {
                target: progressBar
                visible: false
            }
            
            PropertyChanges {
                target: gridForDeviceSelection
                opacity: 1
                visible: true
            }
            
            PropertyChanges {
                target: modelView
                keyNavigationWraps: false
                visible: true
            }
            
            PropertyChanges {
                target: content
                height: 380
            }
        }]
    
    Label {
        id: labelConnectionState
        y: 33
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        font.pointSize: 44
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignHCenter
    }
    
    ProgressBar {
        id: progressBar
        x: 100
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: labelConnectionState.bottom
        anchors.topMargin: 16
        indeterminate: true
    }
    // cancel me...
    Button {
        id: actionButton
        x: 162
        y: 349
        text:  "Cancel"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
    }
        
    GridLayout {
        id: gridForDeviceSelection
        
        width: 300
        visible: false
        
        anchors.bottom: actionButton.top
        anchors.bottomMargin: 12
        anchors.top: labelConnectionState.bottom
        anchors.topMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        
        opacity: 0
        
        ListView {
            id: modelView
            orientation: Qt.Horizontal
            anchors.fill: parent
            visible: false
            
            highlight: Rectangle { 
                color: "lightsteelblue"
                radius: 5
                
            }
            
            focus: true
            
            delegate: Rectangle {
                anchors.fill: parent
                height: 50
                
                border.color: "black"
                border.width: 12
                radius: 14
                                                
                GridLayout {
                    rows: 3
                    columns: 3
                    rowSpacing: 8
                    columnSpacing: 8
                    
                    Image {
                        source: model.modelData.imageSource
                        
                        sourceSize.width: 90
                        sourceSize.height: 90
                        
                        width: 90
                        height: 90
                        
                        fillMode: Image.PreserveAspectFit        
                        Layout.rowSpan: 3
                    }
                    
                    // row 1
                    Label {
                        id: labelSerialNumber
                        text: "Serial Number:"
                        anchors.right: textSerialNumber.left
                        anchors.rightMargin: 12
                        horizontalAlignment: Text.AlignRight                        
                    }
                    
                    Text {
                        id: textSerialNumber
                        text: model.modelData.serialNumber
                        horizontalAlignment: Text.AlignLeft
                    }
                    
                    // row 2
                    Label {
                        id: labelSoftwareVer
                        text: "Software Version:"
                        anchors.right: textSoftwareVer.left
                        anchors.rightMargin: 12
                        horizontalAlignment: Text.AlignRight                        
                    }

                    Text {
                        id: textSoftwareVer
                        text: model.modelData.softwareVersion
                        horizontalAlignment: Text.AlignLeft
                    }
                    
                    // row 3
                    Label {
                        id: labelHardwareVer
                        text: "Hardware Version:"
                        anchors.right: textHardwareVer.left
                        anchors.rightMargin: 12
                        horizontalAlignment: Text.AlignRight                        
                    }

                    Text {
                        id: textHardwareVer
                        text: model.modelData.hardwareVersion
                        horizontalAlignment: Text.AlignLeft
                    }
                    
                }
            }
        }
    }
}

