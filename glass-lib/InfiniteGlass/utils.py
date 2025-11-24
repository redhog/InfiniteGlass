import slugify

def _generate_key_stringify(value):
    if value is None:
        return ""
    if isinstance(value, (list, tuple)):
        return "-".join(_generate_key_stringify(item) for item in value)
    if isinstance(value, bytes):
        try:
            return value.decode("utf-8")
        except:
            pass
    return str(value)

def generate_key(properties, use):
    return slugify.slugify(_generate_key_stringify([properties.get(name, None) for name in sorted(use)]))
