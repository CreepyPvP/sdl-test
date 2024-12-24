#define PI 3.1415

struct v3
{
    f32 X;
    f32 Y;
    f32 Z;

    v3 operator-(v3);
};

inline v3 V3(f32 X)
{
    return {X, X, X};
}

inline v3 V3(f32 X, f32 Y, f32 Z)
{
    return {X, Y, Z};
}

v3 v3::operator-(v3 B)
{
    return V3(X - B.X, Y - B.Y, Z - B.Z);
}

f32 Length(v3 A)
{
    f32 Squared = A.X * A.X + A.Y * A.Y + A.Z * A.Z;
    return sqrt(Squared);
}

struct v2
{
    f32 X;
    f32 Y;
};

inline v2 V2(f32 X)
{
    return {X, X};
}

inline v2 V2(f32 X, f32 Y)
{
    return {X, Y};
}

v3 Norm(v3 A)
{
    f32 Len = Length(A);
    return V3(A.X / Len, A.Y / Len, A.Z / Len);
}

struct mat4
{
    f32 V[16];
};

v3 Cross(v3 A, v3 B)
{
    v3 Result = {};
    Result.X = A.Y * B.Z - A.Z * B.Y;
    Result.Y = A.Z * B.X - A.X * B.Z;
    Result.Z = A.X * B.Y - A.Y * B.X;

    return Result;
}

f32 Dot(v3 A, v3 B) 
{
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

f32 Radians(f32 Degree)
{
    return Degree / 180 * PI;
}

mat4 LookAt(v3 Eye, v3 Target, v3 Up)
{
    mat4 Res = {};

    v3 F = Norm(Target - Eye);
    v3 S = Norm(Cross(F, Up));
    v3 U = Cross(S, F);

    Res.V[0] = S.X;
    Res.V[4] = S.Y;
    Res.V[8] = S.Z;

    Res.V[1] = U.X;
    Res.V[5] = U.Y;
    Res.V[9] = U.Z;

    Res.V[2] = -F.X;
    Res.V[6] = -F.Y;
    Res.V[10] = -F.Z;

    Res.V[12] = -Dot(S, Eye);
    Res.V[13] = -Dot(U, Eye);
    Res.V[14] = Dot(F, Eye);

    Res.V[15] = 1;

    return Res;
}

mat4 Perspective(f32 Fov, f32 Aspect, f32 NearPlane, f32 FarPlane)
{
    mat4 Res = {};

    f32 TanFov = tan(Fov / 2);

    // This one is lefthanded ...
    // Res.V[0] = 1.0f / (TanFov * Aspect);
    // Res.V[5] = 1.0f / (TanFov);
    // Res.V[10] = -(FarPlane + NearPlane) / (FarPlane - NearPlane);
    // Res.V[11] = -1;
    // Res.V[14] = -(2 * NearPlane * FarPlane) / (FarPlane - NearPlane);

    // ... and this one righthanded
    Res.V[0] = 1.0f / (TanFov * Aspect);
    Res.V[5] = 1.0f / (TanFov);
    Res.V[10] = FarPlane / (NearPlane - FarPlane);
    Res.V[11] = -1;
    Res.V[14] = -(NearPlane * FarPlane) / (FarPlane - NearPlane);

    return Res;
}
