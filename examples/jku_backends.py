"""Usage examples for the JKU backends"""

from qiskit_jku_provider import JKUProvider

JKU = JKUProvider()

print(JKU.backends())

backend = JKU.get_backend('local_statevector_simulator_jku')
print(backend)

# gets the name of the backend.
print(backend.name())

# gets the status of the backend.
print(backend.status())

# returns the provider of the backend
print(backend.provider)

# gets the configuration of the backend.
print(backend.configuration())

# gets the properties of the backend.
print(backend.properties())
