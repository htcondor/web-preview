FROM python:3.9

# Install dependencies
COPY requirements.txt /tmp/
RUN pip install --no-cache-dir -r /tmp/requirements.txt

# Copy the script
COPY main.py /app/

# Change the working directory
WORKDIR /app

ENTRYPOINT ["python", "main.py"]