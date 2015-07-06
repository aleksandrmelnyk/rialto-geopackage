from geoserver.wps import process
from com.vividsolutions.jts.geom import Geometry

@process(
    title = 'color_tool',
    description = 'generate a small PNG of a solid color (v1.4)',
    inputs = {
        'color': (string, 'color to use'),
        'outfile': (string, 'output PNG file')
    },
    outputs = {
        'outputt': (string, 'outputt string')
    }
)

def run(color, outfile):
    return 'Hi.'
