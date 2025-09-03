#pragma once
#include <filesystem>
#include <optional>

std::optional<std::filesystem::path> get_save_filename(
    std::string_view filter, std::string_view default_filename,
    const std::filesystem::path& initial_directory, std::string_view caption);

std::optional<std::filesystem::path> get_open_filename(
    std::string_view filter, std::string_view default_filename,
    const std::filesystem::path& initial_directory, std::string_view caption);

bool show_question(std::string_view question_text);

std::optional<std::string> get_string(std::string_view prompt,
                                      std::string_view default_text);
