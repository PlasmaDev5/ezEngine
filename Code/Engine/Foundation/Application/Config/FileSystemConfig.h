#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class EZ_FOUNDATION_DLL ezApplicationFileSystemConfig
{
public:
  static constexpr const ezStringView s_sConfigFile = ":project/RuntimeConfigs/DataDirectories.ddl"_ezsv;

  ezResult Save(ezStringView sPath = s_sConfigFile);
  void Load(ezStringView sPath = s_sConfigFile);

  /// \brief Sets up the data directories that were configured or loaded into this object
  void Apply();

  /// \brief Removes all data directories that were set up by any call to ezApplicationFileSystemConfig::Apply()
  static void Clear();

  ezResult CreateDataDirStubFiles();

  struct DataDirConfig
  {
    ezString m_sDataDirSpecialPath;
    ezString m_sRootName;
    bool m_bWritable;            ///< Whether the directory is going to be mounted for writing
    bool m_bHardCodedDependency; ///< If set to true, this indicates that it may not be removed by the user (in a config dialog)

    DataDirConfig()
    {
      m_bWritable = false;
      m_bHardCodedDependency = false;
    }

    bool operator==(const DataDirConfig& rhs) const
    {
      return m_bWritable == rhs.m_bWritable && m_sDataDirSpecialPath == rhs.m_sDataDirSpecialPath && m_sRootName == rhs.m_sRootName;
    }
  };

  bool operator==(const ezApplicationFileSystemConfig& rhs) const { return m_DataDirs == rhs.m_DataDirs; }

  ezHybridArray<DataDirConfig, 4> m_DataDirs;
};


using ezApplicationFileSystemConfig_DataDirConfig = ezApplicationFileSystemConfig::DataDirConfig;

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezApplicationFileSystemConfig);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezApplicationFileSystemConfig_DataDirConfig);
