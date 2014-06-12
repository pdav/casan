class Enum:
    def __new__(cls, enumName, d):
        e = type(enumName, (object,), d)
        return e
