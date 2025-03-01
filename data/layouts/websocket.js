const obs = new OBSWebSocket();

const max_score = 400;

function clearScores() {
    for(const elem of ["#p1clip", "#p1text", "#p2diff", "#p2clip", "#p2text", "#p1diff"])
    {
        $(elem).hide();
    }
}

function updateScores({p1Score, p2Score, preset}) {
    if (p1Score === undefined || p2Score === undefined) return;

    for(const {score, diff, clip, text, diffText} of [
        {score: p1Score, diff: Math.max(p1Score - p2Score, 0), clip: "#p1clip", text: "#p1text", diffText: "#p2diff"},
        {score: p2Score, diff: Math.max(p2Score - p1Score, 0), clip: "#p2clip", text: "#p2text", diffText: "#p1diff"}])
    {
        if (diff === 0) {
            $(clip).hide();
            $(diffText).hide();
            $(text).removeClass("winning");
        } else {
            const direction = $(clip).attr("data-score-direction");
            let size = diff <= max_score / 25.0 ? 5.0 * diff / max_score : Math.sqrt(diff / max_score);
            if (direction.toLowerCase() === "left" || direction.toLowerCase() === "down") size = 1 - size;
            clippath = {"left": "auto", "up": "auto", "down": "auto", "right": "auto"};
            clippath[direction.toLowerCase()] = String(size * 100) + "%";
            $(clip).css({"clip-path": `rect(${clippath.up} ${clippath.right} ${clippath.down} ${clippath.left})`});
            $(clip).show();
            $(diffText).text(Number(-diff).toLocaleString());
            $(diffText).show();
            $(text).addClass("winning");
        }
        $(text).text(Number(score).toLocaleString());
        $(text).show();
    }
}
$(document).ready(() => {
    clearScores();
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
        setTimeout(() => window.location.reload(), 1000);
    });
});

