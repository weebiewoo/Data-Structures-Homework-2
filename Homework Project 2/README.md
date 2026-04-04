# Polygon Simplification Tool – README

## 📌 Overview

This application simplifies polygon geometry by reducing the number of vertices while attempting to preserve the overall shape.

---

## ⚙️ Requirements

* Windows with **WSL (Windows Subsystem for Linux)** installed
* A compiled executable named `simplify`

---

## 🚀 How to Run

1. **Open your terminal (WSL)**

2. **Navigate to the project directory**

   ```bash
   cd /path/to/your/project
   ```

3. **Run the program using:**

   ```bash
   ./simplify <input_file> <target_vertices> <output_file>
   ```

---

## 🧾 Arguments

* `<input_file>`
  Path to the input polygon file

* `<target_vertices>`
  Desired total number of vertices after simplification

* `<output_file>`
  Path where the simplified polygon will be written

---

## ✅ Example

```bash
./simplify input.csv 100 output.csv
```

This will:

* Read polygon data from `input.csv`
* Simplify it to **100 vertices**
* Write the result to `output.csv`

---

## 📤 Output

The output file will contain:

* Simplified polygon vertices
* Ring and vertex indices
* Summary statistics (area and displacement)

---

## ⚠️ Notes

* The program requires at least **3 vertices**
* Polygons with holes are supported
* Ensure the executable has permission to run:

  ```bash
  chmod +x simplify
  ```

---

## 🛠️ Troubleshooting

* **Permission denied**
  → Run `chmod +x simplify`

* **File not found**
  → Check file paths and current directory

* **Invalid input**
  → Ensure the input file format is correct

---

## 📌 Summary

Just open WSL and run:

```bash
./simplify <input_file> <target_vertices> <output_file>
```

That’s it 👍
