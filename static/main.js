
function startPrediction() {
    document.getElementById('status').innerText = 'Prediction started...';
    fetch('/start_prediction', { method: 'POST' })
        .then(response => response.json())
        .then(data => {
            if (data.error) {
                document.getElementById('status').innerText = `Prediction failed: ${data.error}`;
            } else {
                document.getElementById('predicted_temp').innerText = `🔮 Predicted Temp: ${data.predicted_temperature}°C`;
                document.getElementById('fan_speed').innerText = `🌀 Fan Speed: ${data.fan_speed}%`;
                document.getElementById('status').innerText = 'Prediction completed!';
            }
        })
        .catch(error => {
            document.getElementById('status').innerText = `Prediction failed: ${error.message}`;
        });
}
