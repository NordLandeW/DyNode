#pragma once
enum class IMPORT_XML_RESULT_STATES { SUCCESS, FAILURE, OLD_FORMAT };
IMPORT_XML_RESULT_STATES chart_import_xml(const char* filePath, bool importInfo,
                                          bool importTiming);