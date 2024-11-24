#pragma once

#include <QSystemTrayIcon>
#include <QDialog>
#include <QLabel>
#include <QTimer>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>

class TrayWindow : public QDialog
{
public:
	TrayWindow();
	void UpdateConnectionStatus(bool isConnected);
	bool ShouldConnectAutomatically();
	void SetConnectionButtonHandler(std::function<void(bool)> handler);
private:
	QVBoxLayout mainLayout;
	QLabel statusLabel;
	QPushButton connectButton;
	QCheckBox autoReconnectCheckBox;
	QSystemTrayIcon trayIcon;
	std::function<void(bool)> connectionButtonPressed;
};
