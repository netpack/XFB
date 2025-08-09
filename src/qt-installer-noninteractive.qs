function Controller() {
    installer.setDefaultPageVisible(QInstaller.Introduction, false);
    installer.setDefaultPageVisible(QInstaller.TargetDirectory, true);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, true);
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
    installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, true);
    installer.setDefaultPageVisible(QInstaller.PerformInstallation, true);
    installer.setDefaultPageVisible(QInstaller.FinishedPage, true);
}

function ComponentSelectionPageCallback() {
    var widget = gui.pageWidgetByObjectName("ComponentSelection");
    if (widget) {
        // Select Qt 6.5.1 GCC 64-bit component
        widget.deselectAll();
        widget.selectComponent("qt.qt6.65.gcc_64");
    }
}
