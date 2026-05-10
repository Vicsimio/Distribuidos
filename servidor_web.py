from flask import Flask, Response

app = Flask(__name__)

# Este es el "WSDL" manual para que Zeep no se queje
WSDL_CONTENT = """<?xml version="1.0" encoding="UTF-8"?>
<wsdl:definitions name="ServicioNormalizar"
    targetNamespace="http://localhost:8000/"
    xmlns:tns="http://localhost:8000/"
    xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
    xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
    xmlns:xsd="http://www.w3.org/2001/XMLSchema">
    <wsdl:message name="normalizarRequest">
        <wsdl:part name="mensaje" type="xsd:string"/>
    </wsdl:message>
    <wsdl:message name="normalizarResponse">
        <wsdl:part name="res" type="xsd:string"/>
    </wsdl:message>
    <wsdl:portType name="NormalizarPortType">
        <wsdl:operation name="normalizar_mensaje">
            <wsdl:input message="tns:normalizarRequest"/>
            <wsdl:output message="tns:normalizarResponse"/>
        </wsdl:operation>
    </wsdl:portType>
    <wsdl:binding name="NormalizarBinding" type="tns:NormalizarPortType">
        <soap:binding style="rpc" transport="http://schemas.xmlsoap.org/soap/http"/>
        <wsdl:operation name="normalizar_mensaje">
            <soap:operation soapAction="normalizar_mensaje"/>
            <wsdl:input><soap:body use="literal"/></wsdl:input>
            <wsdl:output><soap:body use="literal"/></wsdl:output>
        </wsdl:operation>
    </wsdl:binding>
    <wsdl:service name="ServicioNormalizar">
        <wsdl:port name="NormalizarPort" binding="tns:NormalizarBinding">
            <soap:address location="http://localhost:8000/"/>
        </wsdl:port>
    </wsdl:service>
</wsdl:definitions>"""

@app.route('/')
def wsdl():
    # Cuando Zeep pida ?wsdl, le damos el XML
    return Response(WSDL_CONTENT, mimetype='text/xml')

@app.route('/', methods=['POST'])
def soap_api():
    # Aquí recibimos la petición de Zeep, limpiamos y devolvemos
    # Para simplificar y que funcione sí o sí, devolvemos un XML SOAP básico
    import re
    from flask import request
    
    data = request.data.decode('utf-8')
    # Extraemos el mensaje del XML (forma rápida)
    match = re.search(r'<mensaje.*?>(.*?)</mensaje>', data)
    mensaje_sucio = match.group(1) if match else ""
    
    # NORMALIZACIÓN (Lo que pide el PDF)
    mensaje_limpio = " ".join(mensaje_sucio.split())
    
    print(f"WS> Recibido: '{mensaje_sucio}' -> Enviado: '{mensaje_limpio}'")
    
    response_xml = f"""<?xml version="1.0" encoding="UTF-8"?>
    <soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
        <soap:Body>
            <normalizar_mensajeResponse>
                <res>{mensaje_limpio}</res>
            </normalizar_mensajeResponse>
        </soap:Body>
    </soap:Envelope>"""
    return Response(response_xml, mimetype='text/xml')

if __name__ == '__main__':
    # Instalación rápida: pip install flask
    app.run(host='0.0.0.0', port=8000)