import boto3
import json
from AWSIoTPythonSDK.MQTTLib import AWSIoTMQTTClient
from boto3.dynamodb.conditions import Key, Attr

print('Loading function')
dynamo = boto3.client('dynamodb')
dynamodb = boto3.resource('dynamodb', endpoint_url="http://localhost:8000")

DYNAMODB_TABLE = "Smart_Agriculture"

table = None

# For certificate based connection
myMQTTClient = AWSIoTMQTTClient("myClientID")
# For Websocket connection
# myMQTTClient = AWSIoTMQTTClient("myClientID", useWebsocket=True)
# Configurations
# For TLS mutual authentication
myMQTTClient.configureEndpoint("a2wez1ijy9ju6g-ats.iot.us-east-1.amazonaws.com", 8883)
# For Websocket
# myMQTTClient.configureEndpoint("YOUR.ENDPOINT", 443)
# For TLS mutual authentication with TLS ALPN extension
# myMQTTClient.configureEndpoint("YOUR.ENDPOINT", 443)
myMQTTClient.configureCredentials("certs/AmazonRootCA1.pem", "certs/3980454d97-private.pem.key", "certs/3980454d97-certificate.pem.crt")
# For Websocket, we only need to configure the root CA
# myMQTTClient.configureCredentials("YOUR/ROOT/CA/PATH")
myMQTTClient.configureOfflinePublishQueueing(-1)  # Infinite offline Publish queueing
myMQTTClient.configureDrainingFrequency(2)  # Draining: 2 Hz
myMQTTClient.configureConnectDisconnectTimeout(10)  # 10 sec
myMQTTClient.configureMQTTOperationTimeout(5)  # 5 sec


def respond(err, res=None):
    return {
        'statusCode': '400' if err else '200',
        'body': err.message if err else json.dumps(res),
        'headers': {
            'Content-Type': 'application/json',
        },
    }

def lambda_handler(event, context):
    '''Demonstrates a simple HTTP endpoint using API Gateway. You have full
    access to the request and response payload, including headers and
    status code.

    To scan a DynamoDB table, make a GET request with the TableName as a
    query string parameter. To put, update, or delete an item, make a POST,
    PUT, or DELETE request respectively, passing in the payload to the
    DynamoDB API as a JSON body.
    '''
    table = dynamodb.Table(DYNAMODB_TABLE)

    print(event)

    print("Received data: " + json.dumps(event, indent=2))

    message = json.dumps(event)

    print(message)
    if "data" in message:
        message = json.loads(message)
        print("Getting garden status!")
        timestamp = message["data"]["timestamp"]
        print("timestamp: ", timestamp)
        temperature = message["data"]["temperature"]
        print("Temperature: ", temperature)
        humidity = message["data"]["humidity"]
        print("humidity: ", humidity)
        update_database(timestamp, temperature, humidity)


def get_all_records():
    response = dynamo.scan(
        TableName=DYNAMODB_TABLE,
        AttributesToGet=[
            'timestamp',
            'temperature',
            'humidity'
        ],
        Select='SPECIFIC_ATTRIBUTES',
    )

    return response


def search_employee(rfid_number):

    response = dynamo.scan(
        TableName=DYNAMODB_TABLE,
        Select='ALL_ATTRIBUTES',
        FilterExpression='#RFID = :RFID_Number',
        ExpressionAttributeNames={
            '#RFID': 'RFID_Number'
        },
        ExpressionAttributeValues={
            ':RFID_Number': {'N': str(rfid_number)},
        },
    )

    print(response)

    return response


def update_database(timestamp, temperature, humidity):

    dynamo.update_item(
        TableName=DYNAMODB_TABLE,
        Key={
            'timestamp': {'N': str(timestamp)},
        },
        UpdateExpression='SET temperature=:temperature, humidity=:humidity',
        ExpressionAttributeValues={
            ':temperature': {'N': str(temperature)},
            ':humidity': {'N': str(humidity)}
        }
    )

def publish_user_prefs(user_prefs):

    myMQTTClient.connect()
    myMQTTClient.publish("GardenActuator", user_prefs, 1)
    myMQTTClient.disconnect()