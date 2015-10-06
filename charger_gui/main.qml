import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Extras 1.4

ApplicationWindow {
    visible: true
    
    width: 640
    height: 480
    
    title: qsTr("Charger")
        
    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
            }
        }
    }
    
    statusBar: StatusBar {
        RowLayout {
            anchors.fill: parent
                        
            Label {
                text: connectionState.connectionMessage
            }
            
            Label {
                Layout.alignment: Qt.AlignRight
                text: {
                    if(!devices.count)
                        return "No devices found"
                    if(devices.count === 1)
                          devices.count + " device";
                    else devices.count + " devices";
                }
            }
        }
    }

    Connecting {
        id: connectionState
        anchors.fill: parent
        deviceListModel.json: devices.jsonData
        cancelButton.onClicked: {
            if(connected) {
                // TODO: Device Request + Selection
                // lets ask the system for all the device information, then we move
                // onto device selection - which is automatic if there's only a single device.
            } else {
                Qt.quit()
            }
        }
        
    }
    
}

