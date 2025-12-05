
function window_on_files_dropped(fileList) {
    var files = json_parse(fileList);
    for(var i = 0; i < array_length(files); i++) {
        var file = files[i];
        var filext = filename_ext(file);

        switch (filext) {
            case ".dyn":
            case ".dy":
            case ".xml":
                project_load(file);
                break;
            case ".jpg":
            case ".jpeg":
            case ".png":
            case ".mp4":
            case ".avi":
            case ".mkv":
                background_load(file);
                break;
            case ".mp3":
            case ".flac":
            case ".wav":
            case ".ogg":
            case ".aiff":
            case ".mid":
                music_load(file);
                break;
            default:
                show_debug_message("!Warning: Unsupported file type dropped: " + filext);
                announcement_warning("drop_unsupported_file");
                break;
        }
    }
}