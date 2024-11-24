#include "TrayWindow.h"
#include <QSystemTrayIcon>
#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>

void TrayWindow::UpdateConnectionStatus(bool isConnected)
{
	statusLabel.setText(isConnected ? "Connected" : "Not connected");
}

bool TrayWindow::ShouldConnectAutomatically()
{
	return autoReconnectCheckBox.isChecked();
}

void TrayWindow::SetConnectionButtonHandler(std::function<void(bool)> handler)
{
	connectionButtonPressed = handler;
}

TrayWindow::TrayWindow() :
	statusLabel("Not connected"),
	connectButton("Connect"),
	autoReconnectCheckBox("Connect automatically"),
	mainLayout(this)
{
	//window.setWindowFlag(Qt::FramelessWindowHint, true);
	setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
	setWindowTitle("Gesture Remote");

	mainLayout.addWidget(&statusLabel);
	mainLayout.addWidget(&connectButton);
	mainLayout.addWidget(&autoReconnectCheckBox);
}