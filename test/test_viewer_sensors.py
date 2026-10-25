import sqlite3
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import pandas as pd
from matplotlib.figure import Figure

from logger import logger, viewer


class ViewerSensorTests(unittest.TestCase):
    def test_value_boxes_display_missing_sensor_values_without_crashing(self):
        for missing in (None, float("nan"), pd.NA):
            with self.subTest(missing=missing):
                axes = Figure().subplots(1, len(viewer.SERIES))
                last = pd.Series({series["col"]: missing for series in viewer.SERIES})
                viewer.draw_value_boxes(axes, last)
                for axis in axes:
                    self.assertIn("--", [text.get_text() for text in axis.texts])

    def test_humidity_overlay_is_dotted_and_preserves_gaps_and_zero(self):
        frame = pd.DataFrame({
            "timestamp": pd.date_range("2026-09-23", periods=3, freq="5s"),
            "humidity_corrected_rh": [95.0, 96.0, 97.0],
            "humidity_scd_rh": [90.0, float("nan"), 0.0],
        })
        axes = Figure().subplots()
        viewer.plot_series(axes, frame, viewer.SERIES[0])
        primary, comparison = axes.lines
        self.assertEqual("Inbox (SHT45)", primary.get_label())
        self.assertEqual("Inbox (SCD41)", comparison.get_label())
        self.assertEqual(":", comparison.get_linestyle())
        self.assertTrue(pd.isna(comparison.get_ydata()[1]))
        self.assertEqual(0.0, comparison.get_ydata()[2])

    def test_ambient_humidity_is_dashed_with_clear_legend_and_missing_gaps(self):
        frame = pd.DataFrame({
            "timestamp": pd.date_range("2026-10-25", periods=3, freq="5s"),
            "humidity_corrected_rh": [40.0, 41.0, 42.0],
            "humidity_scd_rh": [45.0, 46.0, 47.0],
            "humidity_ambient_rh": [50.0, float("nan"), 0.0],
        })
        axis = Figure().subplots()
        with patch.dict(viewer._state, {"adaptive_y": True}):
            viewer.plot_series(axis, frame, viewer.SERIES[0])
        self.assertEqual(["Inbox (SHT45)", "Inbox (SCD41)", "Ambient (SHTC3)"],
                         [line.get_label() for line in axis.lines])
        self.assertEqual("--", axis.lines[2].get_linestyle())
        self.assertTrue(pd.isna(axis.lines[2].get_ydata()[1]))
        self.assertEqual(0.0, axis.lines[2].get_ydata()[2])
        self.assertLess(axis.get_ylim()[0], 40.0)
        self.assertGreater(axis.get_ylim()[1], 50.0)

    def test_temperature_excludes_inbox_pt100(self):
        frame = pd.DataFrame({
            "timestamp": pd.date_range("2026-09-24", periods=3, freq="5s"),
            "temp_box_c": [24.0, 24.1, 24.2],
            "temp_outer_c": [10.0, 10.1, 10.2],
            "temp_ambient_c": [20.0, 20.1, 20.2],
            "temp_box_rtd_c": [25.0, float("nan"), 25.2],
        })
        axes = Figure().subplots()
        viewer.plot_series(axes, frame, viewer.SERIES[2])
        self.assertEqual(["Inbox (SHT45)", "Ambient (SHTC3)"],
                         [line.get_label() for line in axes.lines])
        with patch.dict(viewer._state, {"adaptive_y": True}):
            viewer.set_y_axis(axes, viewer.SERIES[2])
        self.assertLess(axes.get_ylim()[0], 20.0)
        self.assertGreater(axes.get_ylim()[1], 24.2)

    def test_load_old_and_new_database_without_changing_historical_values(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "measurements.db"
            with sqlite3.connect(path) as db:
                db.executescript(logger.SCHEMA)
                db.execute("""INSERT INTO measurements
                    (received_at,boot_id,sequence,uptime_ms,humidity_rh,
                     valid_co2,valid_box,valid_humidity,valid_outer)
                    VALUES ('2026-09-23T12:00:00+02:00',1,1,5000,90,0,0,1,0)""")
            db.close()
            with patch.object(viewer, "DB_FILE", path):
                old = viewer.load_data()
                self.assertEqual(90, old.iloc[0]["humidity_rh"])
                self.assertTrue(pd.isna(old.iloc[0]["humidity_scd_rh"]))
                db = logger.connect(path)
                try:
                    logger.insert(db, {"boot_id": 2, "sequence": 1,
                                       "humidity": 94, "humidity_offset_rh": 1, "scd_humidity": 91})
                finally:
                    db.close()
                current = viewer.load_data()
                self.assertEqual([90.0, 95.0], current["humidity_corrected_rh"].tolist())
                self.assertEqual([90.0, 94.0], current["humidity_rh"].tolist())
                self.assertEqual([91.0], current["humidity_scd_rh"].dropna().tolist())

    def test_large_database_keeps_single_sample_temperature_peak(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "large.db"
            db = sqlite3.connect(path)
            try:
                db.executescript(logger.SCHEMA)
                db.executemany("""INSERT INTO measurements
                    (received_at,boot_id,sequence,uptime_ms,temp_box_c,
                     valid_co2,valid_box,valid_humidity,valid_outer)
                    VALUES ('2026-09-24T00:56:00+02:00',1,?,?,?,0,1,0,0)""",
                    ((sequence, sequence * 5000, 62.83 if sequence == 99999 else 24.6)
                     for sequence in range(1, 100002)))
                db.commit()
            finally:
                db.close()
            with patch.object(viewer, "DB_FILE", path):
                frame = viewer.load_data()
            self.assertEqual(100001, len(frame))
            self.assertEqual(62.83, frame["temp_box_c"].max())
            self.assertEqual(1, (frame["temp_box_c"] == 62.83).sum())

    def test_legacy_csv_without_comparison_column_still_loads(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "old.csv"
            path.write_text("timestamp,co2_ppm,temp_box_c,humidity_rh,temp_outer_c\n"
                            "2026-09-23T12:00:00,800,25,90,20\n")
            with patch.object(viewer, "DB_FILE", path.with_suffix(".db")), \
                    patch.object(viewer, "CSV_FILE", path):
                data = viewer.load_data()
            self.assertEqual(90, data.iloc[0]["humidity_rh"])
            self.assertTrue(pd.isna(data.iloc[0]["humidity_scd_rh"]))

    def test_corrected_humidity_is_capped_and_missing_values_stay_missing(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "corrected.csv"
            path.write_text("timestamp,humidity_rh,humidity_offset_rh\n"
                            "2026-09-24T12:00:00,99.5,1\n"
                            "2026-09-24T12:01:05,,1\n")
            with patch.object(viewer, "DB_FILE", path.with_suffix(".db")), \
                    patch.object(viewer, "CSV_FILE", path):
                data = viewer.load_data()
            self.assertEqual(99.5, data.iloc[0]["humidity_rh"])
            self.assertEqual(100, data.iloc[0]["humidity_corrected_rh"])
            self.assertTrue(pd.isna(data.iloc[1]["humidity_corrected_rh"]))
