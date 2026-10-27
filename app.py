"""Miniestacion IoT: API Flask para guardar y consultar mediciones.
Los datos estan en memoria: desaparecen al reiniciar el servicio.
"""
from flask import Flask, request, jsonify

app = Flask(__name__)
mediciones = []  # Lista donde se acumularan todos los registros recibidos.

# EJEMPLO ENTREGADO: GET /salud responde sin recibir datos.
@app.get("/salud")
def salud():
    return jsonify({"estado": "ok"}), 200


# PASO 1. CREAR GET /mediciones: devolver la lista mediciones.
@app.get("/mediciones")
def consultar_mediciones():
    return jsonify(mediciones), 200


# PASO 2. COMPLETAR POST /mediciones: recibir y guardar registros.
@app.post("/mediciones")
def recibir_mediciones():
    cuerpo = request.get_json(silent=True)
    if not cuerpo or "dispositivo" not in cuerpo or "mediciones" not in cuerpo:
        return jsonify({"error": "Faltan datos"}), 400

    for registro in cuerpo["mediciones"]:
        registro["dispositivo"] = cuerpo["dispositivo"]
        mediciones.append(registro)

    return jsonify({"recibidas": len(cuerpo["mediciones"])}), 201


# FUNCION ENTREGADA: descarga las mediciones desde el navegador.
@app.get("/descargar")
def descargar():
    respuesta = jsonify(mediciones)
    respuesta.headers["Content-Disposition"] = 'attachment; filename="mediciones.json"'
    return respuesta


if __name__ == "__main__":
    app.run(debug=True)
