#!/usr/bin/env bash
# Arma los 3 entregables en entregables/ (ignorada por git):
#   reporte_lab12.pdf    report/reporte_lab12.md -> HTML (pandoc) -> PDF (Chrome sin ventana)
#   evidencia_lab12.pdf  capturas de evidence/screenshots y gráficas de figures, una por página
#   resultados_lab12.zip los 10 CSV de results/raw y los 4 *_con_S_E.csv
# Requiere pandoc, Google Chrome (o Chromium) y zip. Variable CHROME para otra ruta.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/entregables"
TMP="$OUT/tmp"
mkdir -p "$TMP"

if [ -z "${CHROME:-}" ]; then
    for c in "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" \
             "$(command -v google-chrome || true)" "$(command -v chromium || true)"; do
        if [ -n "$c" ] && [ -x "$c" ]; then CHROME="$c"; break; fi
    done
fi
[ -n "${CHROME:-}" ] || { echo "No encontré Chrome; usa CHROME=/ruta/al/navegador" >&2; exit 1; }

a_pdf() {  # a_pdf <html> <pdf>
    "$CHROME" --headless=new --disable-gpu --no-pdf-header-footer \
        --print-to-pdf="$2" "file://$1" 2>/dev/null
}

CSS="$TMP/estilo.css"
cat > "$CSS" <<'EOF'
@page { size: letter; margin: 1.8cm; }
body { font-family: -apple-system, "Helvetica Neue", Arial, sans-serif; font-size: 10.5pt; line-height: 1.4; color: #111; }
h1 { font-size: 20pt; } h2 { font-size: 15pt; border-bottom: 1px solid #999; padding-bottom: 2px; }
h2, h3 { break-after: avoid; }
table { border-collapse: collapse; margin: 8px 0; font-size: 9pt; break-inside: avoid; }
th, td { border: 1px solid #aaa; padding: 3px 6px; }
th { background: #eee; }
img { max-width: 100%; max-height: 10cm; display: block; margin: 8px auto; break-inside: avoid; }
code { font-size: 9pt; background: #f3f3f3; padding: 0 2px; }
blockquote { border-left: 3px solid #999; margin-left: 0; padding-left: 10px; color: #333; }
.pagina { break-after: page; text-align: center; }
.pagina:last-child { break-after: auto; }
.pagina img { max-height: 21cm; }
EOF

# --- 1. Reporte (las imágenes se resuelven relativas a report/) ---
(cd "$ROOT/report" && pandoc reporte_lab12.md -s --math-method=mathml --embed-resources \
    --css "$CSS" --metadata title="Lab 12 — Speedup" -o "$TMP/reporte.html")
# pandoc agrega el título como h1; el reporte ya trae el suyo
sed -i.bak '/<header id="title-block-header">/,/<\/header>/d' "$TMP/reporte.html"
a_pdf "$TMP/reporte.html" "$OUT/reporte_lab12.pdf"

# --- 2. Evidencia: una captura o gráfica por página, con título ---
EVI="$TMP/evidencia.md"
{
    echo "# Lab 12 — Evidencia"
    echo
    echo "Capturas de la laptop donde se corrieron todos los experimentos (i9-13980HX) y las 7 gráficas."
    while IFS='|' read -r archivo titulo; do
        echo
        echo "<div class=\"pagina\">"
        echo
        echo "## $titulo"
        echo
        echo "![]($ROOT/$archivo)"
        echo
        echo "</div>"
    done <<'EOF'
evidence/screenshots/01_lscpu.png|lscpu: modelo de CPU, núcleos, hilos y cachés
evidence/screenshots/02_lscpu_nucleos.png|lscpu -e: núcleos P (CPU 0–15) y núcleos E (CPU 16–31)
evidence/screenshots/03_make.png|Compilación con make y uso de ./lab12
evidence/screenshots/04_amdahl.png|./lab12 amdahl 400000 0.90
evidence/screenshots/05_suma.png|./lab12 suma
evidence/screenshots/06_desbalance_static.png|./lab12 desbalance static
evidence/screenshots/07_desbalance_dynamic.png|./lab12 desbalance dynamic
evidence/screenshots/08_desbalance_guided.png|./lab12 desbalance guided
evidence/screenshots/09_gustafson.png|./lab12 gustafson
figures/amdahl_speedup.png|Parte 1 — Speedup
figures/amdahl_eficiencia.png|Parte 1 — Eficiencia
figures/suma_speedup.png|Parte 2 — Speedup de la suma
figures/desbalance_speedup.png|Parte 3 — Speedup por schedule
figures/desbalance_eficiencia.png|Parte 3 — Eficiencia por schedule
figures/gustafson_speedup.png|Parte 4 — Speedup
figures/gustafson_eficiencia.png|Parte 4 — Eficiencia
EOF
} > "$EVI"
pandoc "$EVI" -s --embed-resources --css "$CSS" --metadata title="Lab 12 — Evidencia" -o "$TMP/evidencia.html"
sed -i.bak '/<header id="title-block-header">/,/<\/header>/d' "$TMP/evidencia.html"
a_pdf "$TMP/evidencia.html" "$OUT/evidencia_lab12.pdf"

# --- 3. ZIP con los 14 CSV ---
rm -f "$OUT/resultados_lab12.zip"
(cd "$ROOT/results" && zip -qj "$OUT/resultados_lab12.zip" raw/*.csv derived/*_con_S_E.csv)

rm -rf "$TMP"
echo "Listo en $OUT:"
ls -lh "$OUT"
echo "CSV en el ZIP: $(unzip -Z1 "$OUT/resultados_lab12.zip" | wc -l | tr -d ' ') (deben ser 14)"
