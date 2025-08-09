// AdView.qml
import QtQuick
import QtWebEngine

WebEngineView {
    id: webView

    // Use anchors to fill the implicit parent provided by QQuickWidget
    // This is generally more robust than relying on parent.width/height directly
    // especially with SizeRootObjectToView mode in QQuickWidget.
    anchors.fill: parent

    // OR, if anchors don't work reliably in this QQuickWidget context,
    // set explicit size initially, maybe based on the known C++ size?
    // width: 300 // Or some reasonable default/expected width
    // height: 90 // Match the C++ fixed height

    // --- Keep the rest ---
    settings.javascriptEnabled: true
    settings.localStorageEnabled: true
    settings.autoLoadImages: true

    property string adHtml: `
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <style> body { margin: 0; padding: 0; overflow: hidden; } </style>
        </head>
        <body>
    <script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-3273285924133768"
         crossorigin="anonymous"></script>
    <!-- XFB_display_responsive -->
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-3273285924133768"
         data-ad-slot="5441850169"
         data-ad-format="auto"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
        </body>
        </html>
    `

    function loadAd() {
        console.log("--- AdView.qml: Loading Ad HTML ---");
        console.log(adHtml);
        console.log("--- AdView.qml: End Ad HTML ---");
        // Ensure webView context is used if calling methods on self
        webView.loadHtml(adHtml, "https://netpack.pt");
    }

    Component.onCompleted: {
        loadAd();
    }
}
