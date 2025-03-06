from flask import Flask, request, jsonify, make_response
from flask_cors import CORS, cross_origin
import random
import time
import threading
import logging

# Nastavení logování
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

app = Flask(__name__)
# Základní CORS nastavení
CORS(app)

# Globální proměnné pro konfiguraci a stav
start_value = 0
stop_value = 25
step_value = 0.2
count_period = 1  # sekundy
is_running = False
histogram_data = []
data_thread = None


# Funkce pro generování dat v samostatném vlákně
def generate_data():
    global histogram_data, is_running
    while is_running:
        # Vygeneruj nová data
        new_data = []

        # Použití numpy.arange místo range pro podporu desetinných kroků
        # Pokud numpy není k dispozici, použijeme vlastní implementaci
        try:
            import numpy as np

            values = np.arange(start_value, stop_value, step_value)
        except ImportError:
            # Vlastní implementace pro desetinné kroky
            values = []
            current = start_value
            while current < stop_value:
                values.append(current)
                current = round(current + step_value, 2)  # Zaokrouhlení na 2 desetinná místa

        for value in values:
            count = random.randint(1, 1000)  # Náhodný počet mezi 1 a 100
            new_data.append([value, count])

        # Aktualizuj globální data
        histogram_data = new_data
        logger.debug(f"Vygenerována nová data: {len(new_data)} položek")

        # Počkej na další periodu
        time.sleep(count_period)


@app.route("/", methods=["GET"])
@cross_origin()
def get_data():
    # Vrať aktuální histogram data
    logger.debug(f"GET požadavek na / - vracím {len(histogram_data)} položek")
    return jsonify(histogram_data)


@app.route("/config", methods=["POST", "OPTIONS"])
@cross_origin()
def set_config():
    logger.debug(f"POST požadavek na /config - data: {request.json}")
    global start_value, stop_value, step_value

    data = request.json
    if not data:
        logger.error("Žádná data nebyla poskytnuta")
        return jsonify({"error": "No data provided"}), 400

    # Aktualizuj konfiguraci
    if "start_value" in data:
        start_value = float(data["start_value"])
    if "stop_value" in data:
        stop_value = float(data["stop_value"])
    if "step_value" in data:
        step_value = float(data["step_value"])

    logger.debug(f"Konfigurace aktualizována: start={start_value}, stop={stop_value}, step={step_value}")
    return jsonify({"start_value": start_value, "stop_value": stop_value, "step_value": step_value})


@app.route("/start", methods=["POST", "OPTIONS"])
@cross_origin()
def start_generation():
    logger.debug(f"POST požadavek na /start - data: {request.json}")
    global is_running, data_thread, count_period

    # Pokud již běží, nejprve zastav
    if is_running:
        logger.warning("Generování dat již běží")
        return jsonify({"error": "Already running"}), 400

    data = request.json
    if data and "count_period" in data:
        count_period = int(data["count_period"])

    # Nastav příznak běhu a spusť vlákno
    is_running = True
    data_thread = threading.Thread(target=generate_data)
    data_thread.daemon = True
    data_thread.start()

    logger.debug(f"Generování dat spuštěno s periodou {count_period}s")
    return jsonify({"status": "started", "count_period": count_period})


@app.route("/stop", methods=["POST", "OPTIONS"])
@cross_origin()
def stop_generation():
    logger.debug("POST požadavek na /stop")
    global is_running, data_thread

    # Zastav generování dat
    is_running = False
    if data_thread:
        data_thread = None

    logger.debug("Generování dat zastaveno")
    return jsonify({"status": "stopped"})


@app.route("/clear", methods=["POST", "OPTIONS"])
@cross_origin()
def clear_data():
    logger.debug("POST požadavek na /clear")
    global histogram_data

    # Vyčisti data
    histogram_data = []

    logger.debug("Data vyčištěna")
    return jsonify({"status": "cleared"})


if __name__ == "__main__":
    logger.info("Aplikace se spouští...")
    app.run(debug=True, host="0.0.0.0", port=8000)
