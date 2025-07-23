

function i18n_load(lname) {
    var b = buffer_load("lang\\"+lname+".json");
    
    if(b < 0) {
        show_error("Localization file reading error. Please check the file integrity.", true);
        return;
    }
    
    var str = json_parse(buffer_read(b, buffer_text));
    buffer_delete(b);
    
    array_push(global.i18nCont, str);
    global.i18nCount ++;
}

function i18n_init() {
    global.i18nLang = 0;
    global.i18nCont = [];
    global.i18nDefault = 0;
    global.i18nCount = 0;
    
    i18n_load("zh-cn");
    i18n_load("zh-tw");
    i18n_load("en-us");
    i18n_load("ja-jp");
    
    i18n_set_lang("zh-cn");
}

function i18n_set_lang(language) {
    if(is_string(language)) {
        for(var i=0, l=global.i18nCount; i<l; i++)
            if(language == global.i18nCont[i].lang) {
                language = i;
                break;
            }
        if(is_string(language)) {
            announcement_error("Language "+language+" not found. Language is set to default.");
            language = global.i18nDefault;
        }
    }
    
    global.i18nLang = language%global.i18nCount;
}

function i18n_get_lang() {
    return global.i18nCont[global.i18nLang].lang;
}

function i18n_get(context, args = []) {
    if(!is_array(args))
        args = [args];

    var _lang = global.i18nLang;
    if(!variable_struct_exists(global.i18nCont[_lang].content, context))
        _lang = global.i18nDefault;
    if(variable_struct_exists(global.i18nCont[_lang].content, context))
        context = variable_struct_get(global.i18nCont[_lang].content, context);

    if(array_length(args) > 0) {
        for(var i=0; i<array_length(args); i++)
            context = string_replace_all(context, "$"+string(i), string(args[i]));
    }
    
    return context;
}

function i18n_get_title() {
    return global.i18nCont[global.i18nLang].title;
}