## Build
### Dependencies
* boost 1.84
* cmake ≥ 3.10

```bash
cmake -S . -B build
```
## Usage
Сервер работает с json-форматом. Пример входного файла:
```json
{
  "resource_name": "resource_info",
  "resource_name2": "resource_info2"
}
```
Для запуска:
```bash
./build/server/server --port <port> --path resources.json
```
```bash
./build/client/client --port <port> --resource <resource_name>
```
