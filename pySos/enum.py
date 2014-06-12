'''
This class implements enumeration behavior
It is used to store a collection of constant value pairs (as long as you 
don't mess with it by hand, but why would you ?)
'''
class Enum:
    def __new__(cls, enumName, d):
        if type(d).__name__ == 'dict':
            e = type(enumName, (object,), d)
            reverse = dict(zip(d.values(), d.keys()))
            d['reverse'] = reverse
            return e
        else:
            dic = dict(zip(d, range(len(d))))
            dicReverse = dict(zip(range(len(d)), d))
            dic['reverse'] = dicReverse
            e = type(enumName, (object,), dic)
            return e
