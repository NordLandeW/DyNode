enum ExprSymbolTypes {
	NUMBER,
	STRING,
	FUNCTION
}

function ExprSymbol(_name="zero", _typ=ExprSymbolTypes.NUMBER, _val=0, _temp=1) constructor {
	name = _name;
	symType = _typ;
	value = undefined;
	tempVar = _temp;
	
	static get_type = function () {
		return symType;
	}
	static get_value = function () {
		return value;
	}
	static set_value = function (_val, _init = false) {
		if(tempVar && !_init)
			throw $"You cannot assign a tempvar {value} with a value {_val}.";
		
		if(is_struct(_val) && !is_method(_val))
			_val = _val.value;
		
		// Value type check
		
		
		if(symType == ExprSymbolTypes.NUMBER) {
			if(is_string(_val) || is_int32(_val) || is_int64(_val) || is_bool(_val))
				_val = real(_val);
			if(!is_real(_val))
				throw $"Symbol {name} is a number, which cannot be set to {_val}";
		}
		if(symType == ExprSymbolTypes.FUNCTION && !is_method(_val))
			throw $"Symbol {name} is a method, which cannot be set to {_val}";
		if(symType == ExprSymbolTypes.STRING && !is_string(_val))
			throw $"Symbol {name} is a string, which cannot be set to {_val}";
		
		value = _val;
	}
	
	static toString = function () {
		return string(value);
	}
	
	set_value(_val, true);
}

function expr_init() {
	if(variable_global_exists("__expr_symbols"))
		delete global.__expr_symbols;
	global.__expr_symbols = {};
	expr_set_func("pow", function(_a, _b) { return power(_a, _b); });
	expr_set_func("sin", function(_a) { return sin(_a); });
	expr_set_func("cos", function(_a) { return cos(_a); });
	expr_set_func("step", function(_edge, _x) { return real(_x >= _edge); });
	expr_set_func("clamp", function(_x, _mn, _mx) { return clamp(_x, _mn, _mx); });
	expr_set_func("exp", function(_x) { return exp(_x); });
	expr_set_func("floor", function(_x) { return floor(_x); });
	expr_set_func("ceil", function(_x) { return ceil(_x); });
	expr_set_func("round", function(_x) { return round(_x); });
	expr_set_func("rand", function(_x) { return random(_x); });
	expr_set_func("randr", function(_l, _r) { return random_range(_l, _r); });
	expr_set_func("irand", function(_x) { return irandom(_x); });
	expr_set_func("irandr", function(_l, _r) { return irandom_range(_l, _r); });
}

function expr_symbol_copy(sym) {
	return new ExprSymbol("tempVar", sym.symType, sym.value, true);
}

function expr_set_var(name, val) {
	variable_struct_set(global.__expr_symbols, name, new ExprSymbol(name, ExprSymbolTypes.NUMBER, val, 0));
}

function expr_set_func(name, fn) {
	variable_struct_set(global.__expr_symbols, name, new ExprSymbol(name, ExprSymbolTypes.FUNCTION, fn, 0));
}

/// @returns {Any} The value of the variable.
function expr_get_var(name) {
	if(!variable_struct_exists(global.__expr_symbols, name))
		throw $"Variable {name} was not defined.";
	return global.__expr_symbols[$ name].value;
}

/// @returns {Struct.ExprSymbol} The symbol of the variable.
function expr_get_sym(name, init_not_existed = false) {
	if(!variable_struct_exists(global.__expr_symbols, name)) {
		if(init_not_existed)
			expr_set_var(name, 0);
		else
			throw $"Variable {name} was not defined before reading.";
	}
		
	return global.__expr_symbols[$ name];
}

function expr_split_args(_args_text) {
	var _args = [];
	var _depth = 0;
	var _seg_start = 1;
	var _len = string_length(_args_text);
	for(var _i = 1; _i <= _len; _i++) {
		var _ch = string_char_at(_args_text, _i);
		if(_ch == "(") _depth++;
		else if(_ch == ")") _depth--;
		else if(_ch == "," && _depth == 0) {
			array_push(_args, string_trim(string_copy(_args_text, _seg_start, _i - _seg_start)));
			_seg_start = _i + 1;
		}
	}
	array_push(_args, string_trim(string_copy(_args_text, _seg_start, _len - _seg_start + 1)));
	if(array_length(_args) == 1 && _args[0] == "")
		return [];
	return _args;
}

function expr_call_func(_name, _args_text) {
	var _fn_sym = expr_get_sym(_name, false);
	if(_fn_sym.symType != ExprSymbolTypes.FUNCTION)
		throw $"Expression error: {_name} is not a function.";
	
	var _arg_exprs = expr_split_args(_args_text);
	var _args = [];
	for(var _i = 0, _l = array_length(_arg_exprs); _i < _l; _i++) {
		var _arg_res = expr_eval(_arg_exprs[_i]);
		array_push(_args, is_struct(_arg_res) ? variable_struct_get(_arg_res, "value") : _arg_res);
	}
	
	var _fn = _fn_sym.value;
	var _ret = 0;
	switch(array_length(_args)) {
		case 0: _ret = _fn(); break;
		case 1: _ret = _fn(_args[0]); break;
		case 2: _ret = _fn(_args[0], _args[1]); break;
		case 3: _ret = _fn(_args[0], _args[1], _args[2]); break;
		case 4: _ret = _fn(_args[0], _args[1], _args[2], _args[3]); break;
		case 5: _ret = _fn(_args[0], _args[1], _args[2], _args[3], _args[4]); break;
		case 6: _ret = _fn(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5]); break;
		case 7: _ret = _fn(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5], _args[6]); break;
		case 8: _ret = _fn(_args[0], _args[1], _args[2], _args[3], _args[4], _args[5], _args[6], _args[7]); break;
		default:
			throw $"Expression error: function {_name} has too many arguments ({array_length(_args)}).";
	}
	return new ExprSymbol("tempFuncRet", ExprSymbolTypes.NUMBER, _ret);
}

function expr_cac(_opt, _a, _b=new ExprSymbol()) {
	if(is_undefined(_a) || is_undefined(_b))
		throw $"Expression error: operator {_opt} need arguments to operate.";
	// show_debug_message_safe($"EXPR CAC {_opt}, {_a}, {_b}")
	var _is_arith_op = (_opt != "=");
	if(_is_arith_op) {
		if(is_struct(_a) && _a.symType == ExprSymbolTypes.FUNCTION)
			throw $"Expression error: function symbol {_a.name} cannot participate in arithmetic operation {_opt}.";
		if(global.__expr_prio[$ _opt][2] != 1 && is_struct(_b) && _b.symType == ExprSymbolTypes.FUNCTION)
			throw $"Expression error: function symbol {_b.name} cannot participate in arithmetic operation {_opt}.";
	}
	var _va = (is_struct(_a)?_a.get_value():_a), _vb = (is_struct(_b)?_b.get_value():_b);
	var _res = new ExprSymbol();
	switch(_opt) {
		case "=":
			_a.set_value(_b);
			_res = expr_symbol_copy(_a);
			_res.tempVar = true;
			break;
		case "+":
			_res = _va + _vb;
			break;
		case "-":
			_res = _va - _vb;
			break;
		case "`+":
			_res = _va;
			break;
		case "`-":
			_res = -_va;
			break;
		case "*":
			_res = _va * _vb;
			break;
		case "/":
			_res = _va / _vb;
			break;
		case "<<":
			_res = _va << _vb;
			break;
		case ">>":
			_res = _va >> _vb;
			break;
		case "%":
			_res = _va % _vb;
			break;
		case "!":
			_res = real(!bool(_va));
			break;
		case ">":
			_res = _va > _vb;
			break;
		case ">=":
			_res = _va >= _vb;
			break;
		case "<":
			_res = _va < _vb;
			break;
		case "<=":
			_res = _va <= _vb;
			break;
		case "==":
			_res = _va == _vb;
			break;
		case "!=":
			_res = _va != _vb;
			break;
		case "&":
			_res = _va & _vb;
			break;
		case "|":
			_res = _va | _vb;
			break;
		case "^":
			_res = _va ^ _vb;
			break;
		case "&&":
			_res = real(bool(_va) && bool(_vb));
			break;
		case "||":
			_res = real(bool(_va) || bool(_vb));
			break;
		default:
			show_debug_message("Warning: unknow operation " + _opt);
			break;
	}
	if(!is_struct(_res))
		_res = new ExprSymbol("TempSymbol", ExprSymbolTypes.NUMBER, _res)
	return _res;
}

global.__expr_prio = {};
///									precedence, associativity, 	unary(1) & both(2)
global.__expr_prio[$ "!"] 	= 		[2, 		1, 				1];
global.__expr_prio[$ "*"] 	= 		[3, 		0, 				0];
global.__expr_prio[$ "/"] 	= 		[3, 		0, 				0];
global.__expr_prio[$ "%"] 	= 		[3, 		0, 				0];
global.__expr_prio[$ "+"] 	= 		[4, 		0, 				2];
global.__expr_prio[$ "-"] 	= 		[4, 		0, 				2];
global.__expr_prio[$ "`+"] 	= 		[4, 		1, 				1];
global.__expr_prio[$ "`-"] 	= 		[4, 		1, 				1];
global.__expr_prio[$ "<<"] 	= 		[5, 		0, 				0];
global.__expr_prio[$ ">>"] 	= 		[5, 		0, 				0];
global.__expr_prio[$ ">" ] 	= 		[6, 		0, 				0];
global.__expr_prio[$ ">="] 	= 		[6, 		0, 				0];
global.__expr_prio[$ "<" ] 	= 		[6, 		0, 				0];
global.__expr_prio[$ "<=" ] =		[6, 		0, 				0];
global.__expr_prio[$ "==" ] =		[7, 		0, 				0];
global.__expr_prio[$ "!=" ] =		[7, 		0, 				0];
global.__expr_prio[$ "&"] 	= 		[8, 		0, 				0];
global.__expr_prio[$ "^"] 	= 		[9, 		0, 				0];
global.__expr_prio[$ "|"] 	= 		[10, 		0, 				0];
global.__expr_prio[$ "&&"] 	= 		[11, 		0, 				0];
global.__expr_prio[$ "||"] 	= 		[12, 		0, 				0];
global.__expr_prio[$ "="] 	= 		[13, 		1, 				0];

///@desc Expression Evaluator
function expr_eval(_expr) {
	_expr += " ";
	// show_debug_message("CAC EXPR "+_expr);

	static _prio = global.__expr_prio;
	var both_to_unary_prefix = "`";
	
	var _stnum = [];
	var _stopt = [];
	// 0 for number, 1 for operator, 2 for bracket, 3 for variable
	var _lastTokenType = -1;
	
	// start analyze
	var _len = string_length(_expr);
	for(var _i = 1; _i <= _len; _i++) {
		var _ch = string_char_at(_expr, _i);
		if(_ch == " ") continue;
		// If a variable
		if(is_alpha(_ch) || _ch == "_") {
			var _j = _i+1;
			_ch = string_char_at(_expr, _j);
			while(is_alpha(_ch) || _ch == "_" || is_number(_ch) || _ch == ".") {
				_j ++;
				_ch = string_char_at(_expr, _j);
			}
			var _varn = string_copy(_expr, _i, _j - _i);
			var _k = _j;
			while(string_char_at(_expr, _k) == " ") _k++;
			if(string_char_at(_expr, _k) == "(") {
				var _fend, _fdepth = 1;
				for(_fend = _k + 1; _fend <= _len; _fend++) {
					_ch = string_char_at(_expr, _fend);
					if(_ch == "(") _fdepth++;
					else if(_ch == ")") _fdepth--;
					if(_fdepth == 0) break;
				}
				if(_fend > _len)
					throw "Expression error: " + _expr + " - Function bracket match error.";
				var _args_text = string_copy(_expr, _k + 1, _fend - _k - 1);
				array_push(_stnum, expr_call_func(_varn, _args_text));
				_i = _fend;
				_lastTokenType = 0;
			}
			else {
				array_push(_stnum, expr_get_sym(_varn, true));
				_i = _j-1;
				_lastTokenType = 3;
			}
		}
		// If a number
		else if(is_number(_ch)) {
			var _j = _i+1;
			_ch = string_char_at(_expr, _j);
			var _subdot = 0;
			while(is_number(_ch) || _ch == ".") {
				_j ++;
				_subdot += (_ch == ".");
				_ch = string_char_at(_expr, _j);
			}
			if(is_alpha(_ch) || _ch == "_")
				throw "Expression error: "+ _expr +" - invalid number.";
			if(_subdot > 1)
				throw "Expression error: "+ _expr +" - invalid real number."
			var _varn = string_copy(_expr, _i, _j - _i);
			array_push(_stnum, new ExprSymbol("tempSym", ExprSymbolTypes.NUMBER, _varn));
			_i = _j-1;
			_lastTokenType = 0;
		}
		// If a bracket
		else if(_ch == "(") {
			var _j, _now = 1;
			for(_j = _i + 1; _j <= _len; _j ++) {
				_ch = string_char_at(_expr, _j);
				if(_ch == "(") _now++;
				else if(_ch == ")") _now--;
				if(_now==0) break;
			}
			if(_j > _len)
				throw "Expression error: " + _expr + " - Bracket match error.";
			var _nexpr = string_copy(_expr, _i+1, _j-_i-1);
			var _nres = expr_eval(_nexpr);
			array_push(_stnum, _nres);
			_i = _j;
			_lastTokenType = 2;
		}
		// If an operation
		else {
			var _opt = _ch;
			_ch = string_char_at(_expr, _i + 1);
			if(variable_struct_exists(_prio, _opt+_ch)) {
				_i ++ ;
				_opt += _ch;
				_ch = string_char_at(_expr, _i + 1);
				if(!(is_alpha(_ch) || _ch = "_" || is_number(_ch) || _ch == "(" || _ch == " "))	
					throw "Expression error: "+ _expr +" - an unexisted operation.";
			}
			if(!variable_struct_exists(_prio, _opt))
				throw "Expression error: "+ _expr +" - an unexisted operation " + _opt + " .";
			
			if(_prio[$ _opt][2] == 2 && _lastTokenType != 0 && _lastTokenType != 3) {
				_opt = both_to_unary_prefix + _opt;
			}
			
			// Caculation
			while(array_length(_stopt)>0) {
				var _prio_now = _prio[$ _opt];
				var _prio_top = _prio[$ array_top(_stopt)];
				var _rtol = _prio_now[0] == _prio_top[0] && _prio_now[1];
				
				if(_rtol || _prio_now[0] < _prio_top[0])
					break;

				var _nopt = array_top(_stopt); array_pop(_stopt);
				var _na = array_top(_stnum); array_pop(_stnum);
				var _ans = undefined;
				if(_prio_top[2] == 1) {
					_ans = expr_cac(_nopt, _na);
				}
				else {
					var _nb = array_top(_stnum); array_pop(_stnum);
					_ans = expr_cac(_nopt, _nb, _na);
				}
				array_push(_stnum, _ans);
			}
			
			array_push(_stopt, _opt);
			
			_lastTokenType = 1;
		}
	}
	
	while(array_length(_stopt)>0) {
		var _nopt = array_top(_stopt); array_pop(_stopt);
		var _na = array_top(_stnum); array_pop(_stnum);
		var _ans = new ExprSymbol();
		if(_prio[$ _nopt][2] == 1) {
			_ans = expr_cac(_nopt, _na);
		}
		else {
			var _nb = array_top(_stnum); array_pop(_stnum);
			_ans = expr_cac(_nopt, _nb, _na);
		}
		array_push(_stnum, _ans);
	}
	
	// Check
	if(array_length(_stnum)>1)
		throw "Expression error: " + _expr;
	var _res = array_top(_stnum);
	
	// show_debug_message("EXPR " + _expr + " RES "+ string(_res));
	
	return _res;
}

///@desc Execute an expression sequence
function expr_exec(expr_seq) {
	var _seqs = string_split(expr_seq, ";", true);
	var _result = new ExprSymbol();
	for(var i=0, l=array_length(_seqs); i<l; i++) {
		try {
			_result = expr_eval(string_trim(_seqs[i]))
		} catch (e) {
			announcement_error(i18n_get("expr_catch_error", [e, _seqs[i]]));
			return -1;
		}
		// announcement_play($"已执行表达式：${_seqs[i]}");
	}
	return 1;
}

function is_alpha(ch) {
	ch=string_lower(ch);
	return in_between(ord(ch), ord("a"), ord("z"))
}

function is_number(ch) {
	return in_between(ord(ch), ord("0"), ord("9"))
}