/// @description Vector2 Class
/// @param {Real} _x The x component of the vector.
/// @param {Real} _y The y component of the vector.
function Vector2(_x = 0, _y = 0) constructor {
    x = _x;
    y = _y;
    
    /// @description Returns the length of this vector.
    /// @returns {Real} The length of this vector.
    static len = function() {
        return sqrt(x*x + y*y);
    }

    /// @description Returns the squared length of this vector.
    /// @returns {Real} The squared length of this vector.
    static len2 = function() {
        return x*x + y*y;
    }

    /// @description Returns a normalized copy of this vector.
    /// @returns {Struct.Vector2} The normalized vector.
    static nrmd = function() {
        var l = len();
        if (l == 0) return new Vector2(0,0);
        return new Vector2(x/l, y/l);
    }

    /// @description Normalizes this vector.
    /// @returns {Struct.Vector2|undefined} This vector after normalization, or undefined if length is zero.
    static nrmz = function() {
        var l = len();
        if (l == 0) return undefined;
        x /= l; y /= l;
        return self;
    }

    /// @description Returns the sum of this vector and another vector.
    /// @param {Struct.Vector2} v The other vector.
    /// @returns {Struct.Vector2} A new vector representing the sum.
    static add = function(v) {
        return new Vector2(x + v.x, y + v.y);
    }

    /// @description Returns the difference of this vector and another vector.
    /// @param {Struct.Vector2} v The other vector.
    /// @returns {Struct.Vector2} A new vector representing the difference.
    static sub = function(v) {
        return new Vector2(x - v.x, y - v.y);
    }

    /// @description Returns this vector multiplied by a scalar.
    /// @param {Real} s The scalar value.
    /// @returns {Struct.Vector2} A new vector scaled by s.
    static mul = function(s) {
        return new Vector2(x * s, y * s);
    }

    /// @description Returns this vector multiplied by another vector element-wise.
    /// @param {Struct.Vector2} v The other vector.
    /// @returns {Struct.Vector2} A new vector with each component multiplied.
    static mulv = function(v) {
        return new Vector2(x * v.x, y * v.y);
    }

    /// @description Returns this vector divided by a scalar.
    /// @param {Real} s The scalar value.
    /// @returns {Struct.Vector2} A new vector scaled by 1/s. Returns (0,0) if s=0.
    static dvd = function(s) {
        if (s == 0) return new Vector2(0,0);
        return new Vector2(x/s, y/s);
    }

    /// @description Returns the dot product of this vector and another.
    /// @param {Struct.Vector2} v The other vector.
    /// @returns {Real} The dot product.
    static dot = function(v) {
        return x*v.x + y*v.y;
    }

    /// @description Returns the distance between this vector and another.
    /// @param {Struct.Vector2} v The other vector.
    /// @returns {Real} The distance between two vectors.
    static dist = function(v) {
        return sqrt((x - v.x)*(x - v.x) + (y - v.y)*(y - v.y));
    }

    /// @description Returns this vector rotated by a given angle in radians.
    /// @param {Real} rad The angle in radians.
    /// @returns {Struct.Vector2} A new rotated vector.
    static rot = function(rad) {
        var cx = cos(rad), sy = sin(rad);
        return new Vector2(x*cx - y*sy, x*sy + y*cx);
    }

    /// @description Returns this vector rotated by a given angle in degrees.
    /// @param {Real} deg The angle in degrees.
    /// @returns {Struct.Vector2} A new rotated vector.
    static rot_d = function(deg) {
        return rot(deg * pi/180);
    }

    /// @description Returns the direction (in degrees) of this vector.
    /// @returns {Real} The direction of this vector in degrees.
    static dir = function() {
        return point_direction(0,0,x,y);
    }

    /// @description Sets the x and y components of this vector.
    /// @param {Real} _x The new x value.
    /// @param {Real} _y The new y value.
    /// @returns {undefined}
    static set = function(_x, _y) {
        x = _x; y = _y;
    }

    /// @description Returns a vector interpolated between this vector and another vector by a factor t.
    /// @param {Struct.Vector2} v The other vector.
    /// @param {Real} t The interpolation factor (0 to 1).
    /// @returns {Struct.Vector2} The interpolated vector.
    static lerp = function(v, t) {
        return new Vector2(x + (v.x - x)*t, y + (v.y - y)*t);
    }

    /// @description Projects this vector onto another vector.
    /// @param {Struct.Vector2} v The vector to project onto.
    /// @returns {Struct.Vector2} The projection of this vector onto v.
    static proj = function(v) {
        var dp = dot(v);
        var v_len2 = v.x*v.x + v.y*v.y;
        if (v_len2 == 0) return new Vector2(0,0);
        var f = dp / v_len2;
        return new Vector2(v.x*f, v.y*f);
    }

    /// @description Returns the projection length of this vector onto another vector.
    /// @param {Struct.Vector2} v The vector to project onto.
    /// @returns {Real} The projection length of this vector onto v.
    static proj_len = function(v) {
        var dp = dot(v);
        var v_len2 = v.x*v.x + v.y*v.y;
        if (v_len2 == 0) return 0;
        return dp / sqrt(v_len2);
    }

    /// @description Reflects this vector around a given normal vector.
    /// @param {Struct.Vector2} n The normal vector (assumed normalized).
    /// @returns {Struct.Vector2} The reflected vector.
    static refl = function(n) {
        var dp = dot(n);
        return new Vector2(x - 2*dp*n.x, y - 2*dp*n.y);
    }

    /// @description Checks if this vector is equal to another vector.
    /// @param {Struct.Vector2} v The other vector.
    /// @returns {Bool} True if equal, false otherwise.
    static eqls = function(v) {
        return (x == v.x && y == v.y);
    }

    /// @description Returns a copy of this vector.
    /// @returns {Struct.Vector2} A copy of this vector.
    static copy = function() {
        return new Vector2(x,y);
    }

    /// @description Creates a vector from a given direction (in degrees) and length.
    /// @param {Real} deg The direction in degrees.
    /// @param {Real} length The length of the new vector.
    /// @returns {Struct.Vector2} A new vector with the given direction and length.
    static fang = function(deg, length) {
        return new Vector2(length * dcos(deg), length * dsin(deg));
    }

    /// @description Returns an array of the vector elements.
    /// @returns {Array<Real>} An array.
    static array = function() {
        return [x, y];
    }

    /// @description Returns a string representation of this vector.
    /// @returns {String} A string representation of the vector.
    static str = function() {
        return "Vector2(" + string(x) + ", " + string(y) + ")";
    }

    static rund = function(step = 1) {
        return new Vector2(round(x / step) * step, round(y / step) * step);
    }

    static flor = function() {
        return new Vector2(floor(x), floor(y));
    }
}

/// @description Vector3 Class
/// @param {Real} _x The x component of the vector.
/// @param {Real} _y The y component of the vector.
/// @param {Real} _z The z component of the vector.
function Vector3(_x = 0, _y = 0, _z = 0) constructor {
    x = _x;
    y = _y;
    z = _z;

    /// @description Returns the length of this vector.
    /// @returns {Real} The length of this vector.
    static len = function() {
        return sqrt(x*x + y*y + z*z);
    }

    /// @description Returns the squared length of this vector.
    /// @returns {Real} The squared length of this vector.
    static len2 = function() {
        return x*x + y*y + z*z;
    }

    /// @description Returns a normalized copy of this vector.
    /// @returns {Struct.Vector3} The normalized vector.
    static nrmd = function() {
        var l = len();
        if (l == 0) return new Vector3(0,0,0);
        return new Vector3(x/l, y/l, z/l);
    }

    /// @description Normalizes this vector.
    /// @returns {Struct.Vector3|undefined} This vector after normalization, or undefined if length is zero.
    static nrmz = function() {
        var l = len();
        if (l == 0) return undefined;
        x /= l; y /= l; z /= l;
        return self;
    }

    /// @description Returns the sum of this vector and another vector.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Struct.Vector3} A new vector representing the sum.
    static add = function(v) {
        return new Vector3(x + v.x, y + v.y, z + v.z);
    }

    /// @description Returns the difference of this vector and another vector.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Struct.Vector3} A new vector representing the difference.
    static sub = function(v) {
        return new Vector3(x - v.x, y - v.y, z - v.z);
    }

    /// @description Returns this vector multiplied by a scalar.
    /// @param {Real} s The scalar value.
    /// @returns {Struct.Vector3} A new vector scaled by s.
    static mul = function(s) {
        return new Vector3(x*s, y*s, z*s);
    }
    
    /// @description Returns this vector multiplied by another vector element-wise.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Struct.Vector3} A new vector with each component multiplied.
    static mulv = function(v) {
        return new Vector3(x*v.x, y*v.y, z*v.z);
    }

    /// @description Returns this vector divided by a scalar.
    /// @param {Real} s The scalar value.
    /// @returns {Struct.Vector3} A new vector scaled by 1/s. Returns (0,0,0) if s=0.
    static dvd = function(s) {
        if (s == 0) return new Vector3(0,0,0);
        return new Vector3(x/s, y/s, z/s);
    }

    /// @description Returns the dot product of this vector and another.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Real} The dot product.
    static dot = function(v) {
        return x*v.x + y*v.y + z*v.z;
    }

    /// @description Returns the cross product of this vector and another.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Struct.Vector3} The cross product vector.
    static crs = function(v) {
        return new Vector3(
            y*v.z - z*v.y,
            z*v.x - x*v.z,
            x*v.y - y*v.x
        );
    }

    /// @description Returns the distance between this vector and another.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Real} The distance between the two vectors.
    static dist = function(v) {
        return sqrt((x - v.x)*(x - v.x) + (y - v.y)*(y - v.y) + (z - v.z)*(z - v.z));
    }

    /// @description Returns a vector interpolated between this vector and another vector by a factor t.
    /// @param {Struct.Vector3} v The other vector.
    /// @param {Real} t The interpolation factor (0 to 1).
    /// @returns {Struct.Vector3} The interpolated vector.
    static lerp = function(v, t) {
        return new Vector3(
            x + (v.x - x)*t,
            y + (v.y - y)*t,
            z + (v.z - z)*t
        );
    }

    /// @description Projects this vector onto another vector.
    /// @param {Struct.Vector3} v The vector to project onto.
    /// @returns {Struct.Vector3} The projection of this vector onto v.
    static proj = function(v) {
        var dp = dot(v);
        var v_len2 = v.x*v.x + v.y*v.y + v.z*v.z;
        if (v_len2 == 0) return new Vector3(0,0,0);
        var f = dp / v_len2;
        return new Vector3(v.x*f, v.y*f, v.z*f);
    }

    /// @description Reflects this vector around a given normal vector.
    /// @param {Struct.Vector3} n The normal vector (assumed normalized).
    /// @returns {Struct.Vector3} The reflected vector.
    static refl = function(n) {
        var dp = dot(n);
        return new Vector3(
            x - 2*dp*n.x,
            y - 2*dp*n.y,
            z - 2*dp*n.z
        );
    }

    /// @description Checks if this vector is equal to another vector.
    /// @param {Struct.Vector3} v The other vector.
    /// @returns {Bool} True if equal, false otherwise.
    static eqls = function(v) {
        return (x == v.x && y == v.y && z == v.z);
    }

    /// @description Returns a copy of this vector.
    /// @returns {Struct.Vector3} A copy of this vector.
    static copy = function() {
        return new Vector3(x,y,z);
    }

    /// @description Returns an array of the vector elements.
    /// @returns {Array<Real>} An array.
    static array = function() {
        return [x, y, z];
    }

    /// @description Returns a string representation of this vector.
    /// @returns {String} A string representation of the vector.
    static str = function() {
        return "Vector3(" + string(x) + ", " + string(y) + ", " + string(z) + ")";
    }
}
