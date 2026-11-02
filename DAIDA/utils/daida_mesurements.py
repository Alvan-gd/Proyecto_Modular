import re
import numpy as np

metrics = {
        "sector_0" : [21.14, 21.02, 22.38, 20.33, 23.05, 19.64, 18.50, 18.87, 20.38, 21.87, 21.21, 20.73, 19.70, 19.92, 20.21, 18.34, 18.66, 19.73, 18.54, 19.00],
        "sector_1" : [48.67, 49.79, 49.55, 50.73, 47.54, 50.50, 48.80, 49.45, 50.62, 50.03, 48.64, 50.06, 48.01, 51.41, 49.13, 49.90, 50.29, 51.18, 52.06, 50.20],
        "sector_2" : [73.23, 74.49, 73.93, 73.79, 74.23, 74.56, 74.28, 74.23, 75.21, 73.30, 73.35, 73.84, 73.71, 72.41, 73.83, 71.15, 72.05, 71.18, 72.01, 72.37],
        "sector_3" : [123.75, 122.30, 111.99, 128.19, 127.37, 125.32, 130.07, 128.44, 127.70, 129.05, 127.46, 129.48, 127.62, 131.02, 131.09, 129.49, 131.68, 126.90, 124.50, 123.00],
        "sector_4" : [161.60, 163.98, 163.46, 169.71, 164.32, 175.34,  155.00, 155.60, 160.75, 161.71, 165.90, 160.97, 165.84, 161.63, 161.84, 158.90, 159.45, 162.21, 159.90, 162.45],
        "sector_5" : [228.08, 240.00, 242.33, 232.45, 248.11, 247.97, 240.01, 240.56, 238.59, 217.83, 227.78, 241.48, 248.43, 235.51, 208.61, 210.41, 213.40, 245.56, 215.78, 221.32],
        "sector_6" : [287.46, 278.72, 283.51, 284.37, 299.62, 286.89, 287.57, 299.50, 291.31, 288.14, 278.62, 284.66, 284.11, 288.56, 278.23, 278.77, 299.89, 286.90, 287.34, 288.01],
        "sector_7" : [335.35, 324.71, 329.77, 330.35, 327.43, 331.91, 333.87, 332.67, 323.56, 326.78, 341.91, 331.45, 336.02, 332.21, 326.90, 341.01, 332.40, 326.34, 322.34, 330.02],
    }

def calcular_metricas_doa(metrics: dict[str, list[float]]) -> None:
  """Procesa el diccionario con llaves 'sector_X' y calcula la estadística direccional

  (Media circular, Bias, MAE, RMSE, Desviación Estándar y Coherencia R).
  """
  nombres_sectores = {
      0: "Frontal",
      1: "Frontal Derecho",
      2: "Derecho",
      3: "Trasero Derecho",
      4: "Trasero",
      5: "Trasero Izquierdo",
      6: "Izquierdo",
      7: "Frontal Izquierdo",
  }

  def extraer_id_sector(clave: str) -> int:
    match = re.search(r"\d+", clave)
    if not match:
      raise ValueError(
          f"No se pudo extraer el índice numérico de la llave: '{clave}'"
      )
    return int(match.group(0))

  encabezados = [
      "Sector",
      "Posición",
      "Nominal (θ)",
      "Med. Estimada",
      "Bias (°)",
      "MAE (°)",
      "RMSE (°)",
      "Std Dev (°)",
      "R (Coh.)",
  ]

  filas = []

  # Ordenar numéricamente por el índice del sector
  claves_ordenadas = sorted(metrics.keys(), key=extraer_id_sector)

  for clave in claves_ordenadas:
    sector_id = extraer_id_sector(clave)
    muestras = np.asarray(metrics[clave], dtype=np.float64)

    if muestras.size == 0:
      continue

    nom = float((sector_id * 45) % 360)
    rads = np.deg2rad(muestras)

    # 1. Componentes vectoriales cartesianos
    C = float(np.mean(np.cos(rads)))
    S = float(np.mean(np.sin(rads)))
    R = float(np.hypot(C, S))

    # 2. Media circular y desviación estándar angular
    if R < 1e-5:
      circ_mean = np.nan
      circ_std = np.nan
    else:
      circ_mean = float(np.rad2deg(np.arctan2(S, C)) % 360.0)
      circ_std = float(
          np.rad2deg(np.sqrt(-2.0 * np.log(np.clip(R, 1e-12, 1.0))))
      )

    # 3. Error angular geodésico de cada muestra respecto al nominal
    diff_nom = np.deg2rad(muestras - nom)
    err_geodesico = np.rad2deg(np.arctan2(np.sin(diff_nom), np.cos(diff_nom)))

    # 4. Sesgo determinista (Bias)
    diff_mean = np.deg2rad(circ_mean - nom)
    bias = float(np.rad2deg(np.arctan2(np.sin(diff_mean), np.cos(diff_mean))))

    # 5. MAE y RMSE
    mae = float(np.mean(np.abs(err_geodesico)))
    rmse = float(np.sqrt(np.mean(err_geodesico**2)))

    posicion = nombres_sectores.get(sector_id, f"Sector {sector_id}")
    filas.append([
        clave,
        posicion,
        f"{nom:5.1f}°",
        f"{circ_mean:5.2f}°",
        f"{bias:+6.2f}°",
        f"{mae:5.2f}°",
        f"{rmse:5.2f}°",
        f"{circ_std:5.2f}°",
        f"{R:6.4f}",
    ])

  # Formateo dinámico y renderizado de la tabla
  anchos = [
      max(len(str(val)) for val in col)
      for col in zip(*([encabezados] + filas))
  ]
  formato_fila = " | ".join(f"{{:<{w}}}" for w in anchos)
  separador = "-+-".join("-" * w for w in anchos)

  print("\n" + formato_fila.format(*encabezados))
  print(separador)
  for fila in filas:
    print(formato_fila.format(*fila))
  print()

if __name__ == "__main__":
    calcular_metricas_doa(metrics)