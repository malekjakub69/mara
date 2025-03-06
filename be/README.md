# Simple Flask API

A simple Flask API that accepts a GET parameter and returns JSON with data.

## Setup

1. Install dependencies:

```
pip install -r requirements.txt
```

2. Run the application:

```
python flask.py
```

## Usage

Send a GET request to `/api/data` with a `data` parameter:

```
http://localhost:5000/api/data?data=hello
```

### Example Response

```json
{
    "data": "hello"
}
```

If no parameter is provided, the default response will be:

```json
{
    "data": "No data provided"
}
```
