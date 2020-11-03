#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

# coding=utf-8

import sys
import subprocess
import math

from PyQt5 import QtGui, QtCore, QtWidgets

def clamp (value, minv, maxv):
  if value < minv:
    return minv
  if value > maxv:
    return maxv
  return value

def get_delta (x):
  start = 0
  y = []
  for value in x:
    y.append (value - start)
    start = value
  return y

def leaky_integrate (x):
  out_value = 0
  y = []
  for in_value in x:
    out_value = out_value * 0.9995 + in_value
    y.append (out_value)
  return y

def get_dc (x):
  dc = 0
  for value in x:
    dc += value
  dc = abs (dc) / len (x)
  if dc > 0:
    return 20 * math.log10 (dc)
  else:
    return -200 # avoid division by zero

def osc_py (params):
  pulse_width = clamp (params["pulse"].value / 100., 0.01, 0.99)
  sub = params["sub"].value / 100.
  shape = params["shape"].value / 100.
  sub_width = clamp (params["subw"].value / 100., 0.01, 0.99)
  sub_width = 1 - sub_width
  sync_factor = 2.0 ** (params["sync"].value / 12.)
  out = []
  pos = 0
  master_phase = 0
  sub_phase = 0
  a = (1 - sub_width) * pulse_width
  c = a + sub_width
  b = c - sub_width * pulse_width
  print ("pw1=%.3f" % (a / (1 - sub_width)))
  print ("pw2=%.3f" % ((c - b) / sub_width))
  print ("sub=%.3f" % (c - a))
  print ("a=%f" % a)
  print ("b=%f" % b)
  print ("c=%f" % c)
  while pos < 4800:
    pos += 1

    master_phase += 0.0005
    if master_phase > 1:
      master_phase -= 1
      sub_phase = 0  # not really correct (see bleposc.hh for a better implementation)

    sub_phase += 0.0005 * sync_factor
    if sub_phase > 1:
      sub_phase -= 1

    saw_state = -4 * sub_phase * (shape + 1)

    if sub_phase > c:
      pout = shape * 4 + 3
      sout = 1
    elif sub_phase > b:
      pout = shape * 2 + 3
      sout = -1
    elif sub_phase > a:
      pout = shape * 2 + 1
      sout = -1
    else:
      pout = 1
      sout = 1

    out.append (saw_state * (1 - sub) + pout * (1 - sub) + sout * sub)

  # out = leaky_integrate (get_delta (out)) # simulate blit integration
  return out

def osc_cxx (params):
  process = subprocess.Popen(['testblep', 'plotblep',
    "%.5f" % params["shape"].value,
    "%.5f" % params["sync"].value,
    "%.5f" % params["sub"].value,
    "%.5f" % params["pulse"].value,
    "%.5f" % params["subw"].value], stdout=subprocess.PIPE)
  out, err = process.communicate()
  plot_data = []
  for o in out.splitlines():
    plot_data += [ float (o) ]
  return plot_data

class BlepWidget(QtWidgets.QWidget):
  def __init__(self):
    super(BlepWidget, self).__init__()
    self.setMinimumSize (100, 100)

  def update_params (self, params, cxx):
    try:
      if cxx:
        self.plot_data = osc_cxx (params)
      else:
        self.plot_data = osc_py (params)
      print ("dc=%f dB" % get_dc (self.plot_data[2000:4000]))
      self.repaint()
    except KeyError:
      # hacky way of avoiding to run testblep if not all params are known
      pass

  def paintEvent(self, event):
    qp = QtGui.QPainter()
    qp.begin(self)
    qp.fillRect (event.rect(), QtGui.QColor (255, 255, 255));
    xgreycolor = QtGui.QColor (240, 240, 240)
    qp.fillRect (QtCore.QRect (0, 0, self.width() / 10, self.height()), xgreycolor)
    qp.fillRect (QtCore.QRect (self.width() * 9 / 10, 0, self.width(), self.height()), xgreycolor)
    xscale = self.width() / 2000 / 1.25
    xcenter = -self.width() / 5 * 3.5
    yscale = -self.height() / 4 * 0.75
    ycenter = self.height() / 2
    qp.setPen (QtGui.QColor (190, 190, 190))
    qp.drawLine (0, ycenter + 1 * yscale, self.width(), ycenter + 1 * yscale)
    qp.drawLine (0, ycenter + -1 * yscale, self.width(), ycenter + -1 * yscale)
    qp.drawLine (self.width() / 2, 0, self.width() / 2, self.height())
    qp.setPen (QtCore.Qt.black)
    for i in range (len (self.plot_data) - 1):
      qp.drawLine (i * xscale + xcenter, self.plot_data[i] * yscale + ycenter, (i + 1) * xscale + xcenter, self.plot_data[i+1] * yscale + ycenter)

    qp.setPen (QtGui.QColor(168, 34, 3))
    qp.drawLine (0, ycenter, self.width(), ycenter)

    qp.end()

class Param:
  pass

class PlotWindow (QtWidgets.QMainWindow):
  def __init__ (self):
    QtWidgets.QMainWindow.__init__(self)

    self.init_ui()

  def init_ui (self):
    central_widget = QtWidgets.QWidget (self)
    self.grid_layout = QtWidgets.QGridLayout()

    self.blep_widget = BlepWidget ()
    self.grid_layout.addWidget (self.blep_widget, 1, 0, 3, 1)
    central_widget.setLayout (self.grid_layout)
    self.setCentralWidget (central_widget)

    self.source_button = QtWidgets.QPushButton ("Source: CXX/Py")

    self.params = dict()
    self.add_slider (1, "shape", 0, -100, 100)
    self.add_slider (2, "sync", 0, 0, 60)
    self.add_slider (3, "pulse", 50, 0, 100)
    self.add_slider (4, "sub", 0, 0, 100)
    self.add_slider (5, "subw", 50, 0, 100)
    self.source_button.setCheckable (True)
    self.source_button.checkable = True
    self.source_button.clicked.connect (self.on_source_clicked)
    self.grid_layout.addWidget (self.source_button, 4, 0, 1, 6)

  def add_slider (self, row, label, vdef, vmin, vmax):
    slider = QtWidgets.QSlider (QtCore.Qt.Vertical)
    slider.setRange (vmin * 10, vmax * 10)
    slider.setValue (vdef * 10)
    slider.setEnabled (True)
    slider.valueChanged.connect (lambda value: self.on_param_changed (label, value))
    self.params[label] = Param()
    self.params[label].value_label = QtWidgets.QLabel()
    self.on_param_changed (label, vdef * 10)
    self.grid_layout.addWidget (slider, 3, row, 1, 1, QtCore.Qt.AlignHCenter)

    self.grid_layout.addWidget (QtWidgets.QLabel (label), 1, row, 1, 1, QtCore.Qt.AlignHCenter)
    self.grid_layout.addWidget (self.params[label].value_label, 2, row, 1, 1, QtCore.Qt.AlignHCenter)
    self.grid_layout.setColumnStretch (0, 1)

  def on_param_changed (self, label, value):
    self.params[label].value = value / 10
    self.params[label].value_label.setText ("%s" % self.params[label].value)
    self.blep_widget.update_params (self.params, self.source_button.isChecked())

  def on_source_clicked (self):
    self.blep_widget.update_params (self.params, self.source_button.isChecked())

def main():
  app = QtWidgets.QApplication (sys.argv)
  plot = PlotWindow()
  plot.show()
  sys.exit (app.exec_())

if __name__ == "__main__":
  main()
