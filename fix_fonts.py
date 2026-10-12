import os

for root, _, files in os.walk('web/src'):
    for file in files:
        if file.endswith(('.vue', '.css')):
            path = os.path.join(root, file)
            with open(path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            if 'Bar Roboto' in content:
                content = content.replace("'Bar Roboto'", "'Roboto', sans-serif")
                content = content.replace('"Bar Roboto"', '"Roboto", sans-serif')
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(content)
                print(f"Исправлен шрифт в {path}")
