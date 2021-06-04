studio.plugins.registerPluginDescription("Modal Synth", {
    companyName: "Runic Sounds",
    productName: "Modal Synth",
    deckUi: {
        deckWidgetType: studio.ui.deckWidgetType.Layout,
        layout: studio.ui.layoutType.HBoxLayout,
        spacing: 6,
        items: [
            { deckWidgetType: studio.ui.deckWidgetType.DataDrop, binding: "Data", fileNameFilters: "*.csv" },
        ],
    },
});