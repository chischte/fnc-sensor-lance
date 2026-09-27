import unittest
from unittest.mock import patch

import pandas as pd
from matplotlib import dates as mdates
from matplotlib.figure import Figure

from logger import viewer


class ViewerRefreshTests(unittest.TestCase):
    def setUp(self):
        self.figure = Figure()
        self.boxes = [self.figure.add_subplot(3, 3, i + 1) for i in range(3)]
        self.charts = [self.figure.add_subplot(3, 3, i + 4) for i in range(3)]
        self.range_axis = self.figure.add_subplot(3, 1, 3)
        self.footer = self.figure.text(0, 0, "")
        self.frame = pd.DataFrame({
            "timestamp": pd.date_range("2026-09-27T12:00:00", periods=3, freq="min"),
            "humidity_corrected_rh": [88.0, 88.1, 88.2],
            "co2_ppm": [1000, 1100, 1200],
            "temp_box_c": [25.0, 25.1, 25.2],
        })
        self.x_limits = tuple(mdates.date2num(self.frame["timestamp"].iloc[:2]))
        self.y_limits = [(87.5, 88.5), (900, 1300), (24.9, 25.3)]
        for axis, limits in zip(self.charts, self.y_limits):
            axis.set_xlim(self.x_limits)
            axis.set_ylim(limits)

    def refresh(self, preserve_view=True, adaptive_y=False):
        with patch.object(viewer, "load_data", return_value=self.frame), \
                patch.dict(viewer._state, {"range": "30m", "adaptive_y": adaptive_y}):
            viewer.draw(self.figure, self.boxes, self.charts, self.range_axis,
                        self.footer, preserve_view=preserve_view)

    def test_refresh_preserves_horizontal_and_vertical_zoom_with_new_data(self):
        for _ in range(2):
            self.refresh()
            for axis, limits in zip(self.charts, self.y_limits):
                self.assertEqual(self.x_limits, axis.get_xlim())
                self.assertEqual(limits, axis.get_ylim())
            self.frame.loc[len(self.frame)] = [
                self.frame["timestamp"].iloc[-1] + pd.Timedelta(minutes=1),
                90.0, 2000, 27.0,
            ]

    def test_adaptive_y_uses_visible_data_and_preserves_time_zoom(self):
        self.refresh(adaptive_y=True)
        for axis, limits in zip(self.charts, self.y_limits):
            self.assertEqual(self.x_limits, axis.get_xlim())
            self.assertNotEqual(limits, axis.get_ylim())

    def test_range_selection_restores_default_scales(self):
        self.refresh(preserve_view=False)
        for axis, series in zip(self.charts, viewer.SERIES):
            self.assertNotEqual(self.x_limits, axis.get_xlim())
            self.assertEqual((series["ymin"], series["ymax"]), axis.get_ylim())
