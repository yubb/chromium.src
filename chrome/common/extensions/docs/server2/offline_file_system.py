# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from file_system import FileSystem, FileNotFoundError

class OfflineFileSystem(FileSystem):
  '''An offline FileSystem which masquerades as another file system. It throws
  FileNotFound error for all operations, and overrides GetName and GetVersion.
  '''
  def __init__(self, cls):
    self._cls = cls

  def Read(self, paths, binary=False):
    raise FileNotFoundError(paths)

  def Stat(self, path):
    raise FileNotFoundError(path)

  # HACK: despite GetName/GetVersion being @classmethods, these need to be
  # instance methods so that we can grab the name and version from the class
  # given on construction.

  def GetName(self):
    return self._cls.GetName()

  def GetVersion(self):
    return self._cls.GetVersion()
