import argparse
import csv
import math
from collections import defaultdict
from html import escape
from pathlib import Path
from statistics import median


OPERATIONS = [
    ("build_ms", "Построение через Set"),
    ("bulk_ms", "Массовое построение"),
    ("read_ms", "Чтение ненулевых значений"),
    ("missing_ms", "Чтение нулевых позиций"),
    ("update_ms", "Обновление существующих записей"),
    ("remove_ms", "Удаление записью нуля"),
    ("map_ms", "Map identity"),
    ("where_ms", "Where positive"),
    ("reduce_ms", "Reduce xor"),
]
STORAGES = [("tree", "AVL", "#1768ac"), ("array", "Массив", "#c65b16")]
ORDERS = [("random", "случайный", ""), ("ascending", "возрастающий", "8 4"),
          ("descending", "убывающий", "2 4")]


def read_results(path, x_column):
    groups = defaultdict(list)
    with path.open(encoding="utf-8", newline="") as source:
        reader = csv.DictReader(source)
        required = {"length", "nonzero", "storage", "order", *(column for column, _ in OPERATIONS)}
        if not required.issubset(reader.fieldnames or []):
            raise ValueError("CSV не содержит необходимые столбцы бенчмарка")
        for row in reader:
            length, count = int(row["length"]), int(row["nonzero"])
            if not 0 <= count <= length:
                raise ValueError("Неверные N или k в CSV")
            if row["storage"] not in {item[0] for item in STORAGES} or row["order"] not in {item[0] for item in ORDERS}:
                raise ValueError("Неизвестное хранилище или порядок в CSV")
            times = {column: float(row[column]) for column, _ in OPERATIONS}
            if any(not math.isfinite(value) or value < 0 for value in times.values()):
                raise ValueError("Времена должны быть конечными неотрицательными числами")
            x = int(row[x_column])
            if x > 0:
                groups[(row["storage"], row["order"], x)].append((length, count, times))
    if not groups:
        raise ValueError("Для логарифмических графиков нужны записи с положительным размером")
    for rows in groups.values():
        if len({(length, count) for length, count, _ in rows}) != 1:
            raise ValueError("Одному x соответствуют разные N и k; разделите эксперименты на разные CSV")
    return groups


def draw_graphs(groups, title, x_column):
    width, height = 1470, 1120
    other_index = 0 if x_column == "nonzero" else 1
    other_values = {rows[0][other_index] for rows in groups.values()}
    other_name = "N" if x_column == "nonzero" else "k"
    dimension = f"{other_name} = {next(iter(other_values)):,}".replace(",", " ") if len(other_values) == 1 else f"{other_name} меняется, см. CSV"
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
           f'viewBox="0 0 {width} {height}" role="img" aria-label="Сравнение хранилищ разреженного вектора">',
           '<style>text{font-family:Arial,sans-serif;fill:#243244} .tick{font-size:11px} '
           '.caption{font-size:14px;font-weight:bold}</style>',
           f'<rect width="{width}" height="{height}" fill="white"/>',
           f'<text x="40" y="32" font-size="22">{escape(title)}</text>',
           f'<text x="40" y="55" font-size="13">Медиана повторов; время всего пакета в мс; обе оси логарифмические; {dimension}</text>']
    for storage_index, (_, storage_name, color) in enumerate(STORAGES):
        for order_index, (_, order_name, dash) in enumerate(ORDERS):
            x = 40 + order_index * 260
            y = 80 + storage_index * 22
            svg.append(f'<line x1="{x}" y1="{y}" x2="{x + 35}" y2="{y}" stroke="{color}" '
                       f'stroke-width="2" stroke-dasharray="{dash}"/>')
            svg.append(f'<text x="{x + 44}" y="{y + 4}" font-size="12">{storage_name}, {order_name}</text>')

    x_values = [key[2] for key in groups]
    x_min = math.floor(math.log10(min(x_values)))
    x_max = max(x_min + 1, math.ceil(math.log10(max(x_values))))
    x_label = "Ненулевых элементов k" if x_column == "nonzero" else "Длина вектора N"
    for panel, (column, caption) in enumerate(OPERATIONS):
        left = 78 + (panel % 3) * 485
        top = 158 + (panel // 3) * 308
        plot_width, plot_height = 365, 215
        points = defaultdict(list)
        for (storage, order, x), rows in groups.items():
            value = median(times[column] for _, _, times in rows)
            if value > 0:
                points[(storage, order)].append((x, value))
        positive = [value for series in points.values() for _, value in series]
        y_min = math.floor(math.log10(min(positive))) if positive else -3
        y_max = max(y_min + 1, math.ceil(math.log10(max(positive)))) if positive else 0

        def px(value):
            return left + (math.log10(value) - x_min) * plot_width / (x_max - x_min)

        def py(value):
            return top + plot_height - (math.log10(value) - y_min) * plot_height / (y_max - y_min)

        svg.append(f'<text x="{left}" y="{top - 18}" class="caption">{escape(caption)}</text>')
        for power in range(x_min, x_max + 1):
            x = px(10 ** power)
            svg.append(f'<line x1="{x:.2f}" y1="{top}" x2="{x:.2f}" y2="{top + plot_height}" stroke="#e4e9ef"/>')
            svg.append(f'<text x="{x:.2f}" y="{top + plot_height + 18}" text-anchor="middle" class="tick">{10 ** power:g}</text>')
        for power in range(y_min, y_max + 1):
            y = py(10 ** power)
            svg.append(f'<line x1="{left}" y1="{y:.2f}" x2="{left + plot_width}" y2="{y:.2f}" stroke="#e4e9ef"/>')
            svg.append(f'<text x="{left - 9}" y="{y + 4:.2f}" text-anchor="end" class="tick">{10 ** power:g}</text>')
        svg.append(f'<rect x="{left}" y="{top}" width="{plot_width}" height="{plot_height}" fill="none" stroke="#b5bfcd"/>')
        for storage, _, color in STORAGES:
            for order, _, dash in ORDERS:
                series = sorted(points[(storage, order)])
                if not series:
                    continue
                coordinates = " ".join(f"{px(x):.2f},{py(value):.2f}" for x, value in series)
                svg.append(f'<polyline points="{coordinates}" fill="none" stroke="{color}" stroke-width="2" '
                           f'stroke-dasharray="{dash}"/>')
                for x, value in series:
                    label = escape(f"{storage}, {order}: {x}, {value:.6g} ms")
                    svg.append(f'<circle cx="{px(x):.2f}" cy="{py(value):.2f}" r="3" fill="{color}"><title>{label}</title></circle>')
        svg.append(f'<text x="{left + plot_width / 2}" y="{top + plot_height + 43}" text-anchor="middle" font-size="12">{x_label}</text>')
        svg.append(f'<text x="{left - 12}" y="{top - 18}" text-anchor="end" class="tick">мс</text>')
        if not positive:
            svg.append(f'<text x="{left + 15}" y="{top + 100}" font-size="12">Нет положительных замеров</text>')
    svg.append('<text x="40" y="1100" font-size="12">Чтение нулей использует min(k, N − k) запросов; остальные пакеты — k операций или один полный проход. Нулевые времена не показаны.</text>')
    svg.append('</svg>')
    return "\n".join(svg)


def main():
    parser = argparse.ArgumentParser(description="Графики замеров без сторонних пакетов Python")
    parser.add_argument("csv", nargs="?", default="results/benchmark.csv", type=Path)
    parser.add_argument("--x", choices=["nonzero", "length"], default="nonzero")
    args = parser.parse_args()
    try:
        groups = read_results(args.csv, args.x)
        graph = draw_graphs(groups, "SparseVector — AVL и отсортированный массив", args.x)
        output = args.csv.with_suffix(".svg")
        output.write_text(graph, encoding="utf-8")
        print(f"Графики сохранены: {output}")
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
