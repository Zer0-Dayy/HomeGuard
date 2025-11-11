from flask import Flask, request


app = Flask(__name__)

@app.route('/data',methods=['POST','GET'])
def receive_data():
	if request.method == 'POST':
		data = request.data.decode('utf-8')
		print(f"Received data: {data}")
		return "Data received\n",200
	else:
		return "Send data with POST.\n",200


if __name__ == '__main__':
	app.run(host='0.0.0.0', port=5000)