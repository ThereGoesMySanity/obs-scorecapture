$(document).ready(() => {
    clearScores();
});
if (!window.obsstudio) {
    const obs = new OBSWebSocket();
    obs.connect("ws://localhost:4455", "", {
        eventSubscriptions: OBSWebSocket.EventSubscription.Vendors
    }).then(() => {
        obs.on('VendorEvent', ({vendorName, eventType, eventData}) => {
            if (vendorName === "ScoreCapture") {
                switch (eventType) {
                    case "scores_updated":
                        updateScores(eventData);
                        break;
                    case "scores_cleared":
                        clearScores();
                        break;
                }
            }
        });
    }, () => {
        console.log("connection failed, refreshing in 1s");
        //setTimeout(() => window.location.reload(), 1000);
    });
} else {
    window.addEventListener('scores_updated', e => updateScores(e.detail))
    window.addEventListener('scores_cleared', clearScores);
}
