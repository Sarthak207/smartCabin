
from flask import Flask, request, jsonify, render_template
import joblib
import numpy as np
import firebase_admin
from firebase_admin import credentials, db

app = Flask(__name__)

# Load the trained ML model and scaler
model = joblib.load("train.pkl")
scaler = joblib.load("scaler.pkl")  # Load the saved scaler

# Initialize Firebase (Ensure you have your service account JSON file)
cred = credentials.Certificate("firebase_credentials.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://your-url.firebaseio.com'
})

@app.route('/')
def index():
    return render_template('index2.html')

@app.route('/start_prediction', methods=['POST'])
def start_prediction():
    # Fetch real-time data from Firebase
    ref = db.reference("HolisticCabin")
    data = ref.get()

    if data and 'Temperature' in data and 'Humidity' in data:
        current_temp = data['Temperature']
        current_humidity = data['Humidity']

        # Prepare input data
        input_data = np.array([[current_humidity, current_temp]])  # Format it correctly

        # Scale input data
        input_scaled = scaler.transform(input_data)

        # Predict temperature & humidity
        prediction = model.predict(input_scaled)

        # Extract predictions
        predicted_temp = prediction[0][0].item()
        predicted_humidity = prediction[0][1].item()

        # Clip values to required range
        predicted_temp = round(np.clip(predicted_temp, 21, 27),2)  # Ensuring temp is within 21-27°C
        predicted_humidity = np.clip(predicted_humidity, 25, 35)  # Ensuring humidity is within 25-35%

        # Calculate fan speed (Example: Scale based on the difference between predicted & current temp)
        fan_speed = round(min(100, max(0, ((current_temp - predicted_temp) / 15) * 100)))

        # Update Firebase with predicted values
        ref.update({
            "predicted_temperature": predicted_temp,
            "predicted_humidity": predicted_humidity,
            "fan_speed": fan_speed
        })

        return jsonify({
            "current_temperature": current_temp,
            "current_humidity": current_humidity,
            "predicted_temperature": predicted_temp,
            "predicted_humidity": predicted_humidity,
            "fan_speed": fan_speed
        })
    else:
        return jsonify({"error": "Temperature or Humidity not found in Firebase"}), 400

if __name__ == '__main__':
    app.run(debug=True, host="0.0.0.0", port=5000)
