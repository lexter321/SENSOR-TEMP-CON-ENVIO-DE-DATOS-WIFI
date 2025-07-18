from flask import Flask, request, jsonify
import datetime

app = Flask(__name__)

@app.route('/dht_data', methods=['POST'])
def receive_dht_data():
    if request.is_json:
        data = request.get_json()
        temperatura = data.get('temperatura')
        humedad = data.get('humedad')
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        print(f"[{timestamp}] Datos recibidos del ESP32:")
        print(f"  Temperatura: {temperatura} *C")
        print(f"  Humedad: {humedad} %")
        print("-" * 30)

        # Opcional: Guardar en un archivo de log
        with open("dht_log.txt", "a") as f:
            f.write(f"[{timestamp}] Temp: {temperatura}C, Hum: {humedad}%\n")

        return jsonify({"status": "success", "message": "Datos recibidos"}), 200
    else:
        print(f"[{datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}] Error: Petición no es JSON")
        return jsonify({"status": "error", "message": "Content-Type must be application/json"}), 400

@app.route('/')
def index():
    return "Servidor DHT de Termux. Enviando POST a /dht_data"

if __name__ == '__main__':
    # Asegúrate de que el host sea '0.0.0.0' para que sea accesible desde otros dispositivos en la red
    # y el puerto '5000' para que coincida con el código del ESP32.
    app.run(host='0.0.0.0', port=80)