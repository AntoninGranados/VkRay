[__AttributeUsage(_AttributeTargets.Var)]
struct RangeAttribute {
    float min;
    float max;
};

[__AttributeUsage(_AttributeTargets.Var)]
struct MinAttribute {
    float value;
};

[__AttributeUsage(_AttributeTargets.Var)]
struct MaxAttribute {
    float value;
};

[__AttributeUsage(_AttributeTargets.Var)]
struct ColorAttribute {};

[__AttributeUsage(_AttributeTargets.Var)]
struct AnimatableAttribute {};

[__AttributeUsage(_AttributeTargets.Struct)]
struct PassCountAttribute {
    int count;
};
