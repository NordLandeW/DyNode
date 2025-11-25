#pragma once

inline constexpr int XML_EXPORT_EPS = 6;

enum class IMPORT_XML_RESULT_STATES { SUCCESS, FAILURE, OLD_FORMAT };
IMPORT_XML_RESULT_STATES chart_import_xml(const char* filePath, bool importInfo,
                                          bool importTiming);
void chart_export_xml(const char* filePath, bool isDym, double fixError);
