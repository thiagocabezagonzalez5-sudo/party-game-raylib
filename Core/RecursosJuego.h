#pragma once


//==================================================
// RUTAS COMPARTIDAS DE RECURSOS
//==================================================
//
// Este archivo es la unica fuente de verdad para las rutas de Assets.
// Si un recurso cambia de carpeta o de nombre, se corrige aqui y no en
// cada minijuego/pantalla que lo utiliza.
//==================================================

inline constexpr const char* DIRECTORIO_RECURSOS =
    "Assets";

inline constexpr const char* RUTA_CONFIGURACION_JUEGO =
    "config.ini";


//==================================================
// MODELO GENERAL DEL JUGADOR
//==================================================

inline constexpr const char* RUTA_MODELO_JUGADOR_3D =
    //"Assets/Modelos/Jugador_Raylib_Normalizado.glb";
    "Assets/Modelos/tung_tung_tung_sahur.glb";

inline constexpr float ESCALA_MODELO_JUGADOR_3D =
    0.25f;

inline constexpr float ROTACION_X_MODELO_JUGADOR_3D =
    0.0f;


//==================================================
// MODELOS DE ESCENARIOS DE MINIJUEGOS
//==================================================

// Este archivo es opcional. Mientras no exista, Color Seguro conserva las
// montanas low-poly actuales. Al agregar el GLB, todas las instancias del
// fondo se reemplazan automaticamente por este modelo compartido.
inline constexpr const char* RUTA_MODELO_MONTANA_LAVA_3D =
    "Assets/Modelos/Escenarios/ColorSeguro/MontanaLava.glb";

// Permite corregir un modelo exportado con otro eje vertical sin tocar el
// codigo del escenario. Para un GLB preparado con Y hacia arriba se deja 0.
inline constexpr float ROTACION_X_MODELO_MONTANA_LAVA_3D =
    0.0f;

// Orden del paquete original (manifest.json). Cada ID identifica un recurso,
// no una instancia: caballitos, cabinas y tazas comparten su respectivo GLB.
enum ModeloUltimoAsiento3D
{
    MODELO_ASIENTO_CARRUSEL_BASE,
    MODELO_ASIENTO_CARRUSEL_COLUMNA,
    MODELO_ASIENTO_CARRUSEL_TECHO,
    MODELO_ASIENTO_CABALLITO,
    MODELO_ASIENTO_POSTE_CABALLITO,
    MODELO_ASIENTO_BOMBILLA,
    MODELO_ASIENTO_TAZA,
    MODELO_ASIENTO_VALLA,
    MODELO_ASIENTO_NORIA_SOPORTE,
    MODELO_ASIENTO_NORIA_RUEDA,
    MODELO_ASIENTO_NORIA_CABINA,
    MODELO_ASIENTO_MONTANA_VIAS,
    MODELO_ASIENTO_MONTANA_CARRO,
    MODELO_ASIENTO_PUESTO,
    MODELO_ASIENTO_GRADA,
    MODELO_ASIENTO_GLOBO,
    MODELO_ASIENTO_ARENA,
    CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D
};

inline constexpr const char* RUTAS_MODELOS_ULTIMO_ASIENTO_3D[] =
{
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/carrusel_base.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/carrusel_columna.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/carrusel_techo.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/caballito.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/poste_caballito.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/bombilla.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/taza.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/valla_tramo.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/noria_soporte.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/noria_rueda.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/noria_cabina.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/montana_rusa_vias.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/montana_rusa_carro.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/puesto_feria.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/grada.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/globo.glb",
    "Assets/Modelos/Escenarios/Ultimo_Asiento/GLB/arena.glb"
};

static_assert(sizeof(RUTAS_MODELOS_ULTIMO_ASIENTO_3D) /
    sizeof(RUTAS_MODELOS_ULTIMO_ASIENTO_3D[0]) == CANTIDAD_MODELOS_ULTIMO_ASIENTO_3D,
    "Las rutas deben coincidir con los IDs del paquete Ultimo Asiento");

// Un recurso por pieza del puerto, compartido por las ocho instancias.
enum ModeloCajasPuerto3D
{
    MODELO_CAJAS_MUELLE,
    MODELO_CAJAS_GRUA_PORTICO,
    MODELO_CAJAS_GRUA_CARRO,
    MODELO_CAJAS_GRUA_CABINA,
    MODELO_CAJAS_GRUA_BRAZO,
    MODELO_CAJAS_GRUA_CABLE,
    MODELO_CAJAS_GRUA_GANCHO,
    MODELO_CAJAS_CONTENEDOR_CUERPO,
    MODELO_CAJAS_PUERTA_IZQUIERDA,
    MODELO_CAJAS_PUERTA_DERECHA,
    MODELO_CAJAS_CONTENEDOR_DECORACION,
    MODELO_CAJAS_FAROL,
    MODELO_CAJAS_BOLARDO,
    MODELO_CAJAS_ANCLA,
    MODELO_CAJAS_BARCO,
    MODELO_CAJAS_MARCA_GOLPE,
    CANTIDAD_MODELOS_CAJAS_PUERTO_3D
};

inline constexpr const char* RUTAS_MODELOS_CAJAS_PUERTO_3D[] =
{
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/muelle.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/grua_portico.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/grua_carro.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/grua_cabina.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/grua_brazo.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/grua_cable_unidad.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/grua_gancho.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/contenedor_cuerpo.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/contenedor_puerta_izquierda.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/contenedor_puerta_derecha.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/contenedor_decoracion.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/farol.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/bolardo.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/ancla_dorada.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/barco_fondo.glb",
    "Assets/Modelos/Escenarios/Cajas_del_Puerto/GLB/marca_golpe.glb"
};

static_assert(sizeof(RUTAS_MODELOS_CAJAS_PUERTO_3D) /
    sizeof(RUTAS_MODELOS_CAJAS_PUERTO_3D[0]) == CANTIDAD_MODELOS_CAJAS_PUERTO_3D,
    "Las rutas deben coincidir con los IDs del paquete Cajas del Puerto");

enum ModeloLaberintoJade3D
{
    MODELO_JADE_LOSA,
    MODELO_JADE_BANDA,
    MODELO_JADE_MURO,
    MODELO_JADE_GLIFO,
    MODELO_JADE_AGUJERO,
    MODELO_JADE_SALIDA,
    MODELO_JADE_CHECKPOINT,
    MODELO_JADE_ALTAR,
    MODELO_JADE_ESFERA,
    MODELO_JADE_FLECHA,
    MODELO_JADE_BOQUILLA,
    MODELO_JADE_TEMPLO,
    MODELO_JADE_COLUMNA,
    MODELO_JADE_ANTORCHA,
    MODELO_JADE_LLAMA,
    CANTIDAD_MODELOS_LABERINTO_JADE_3D
};

inline constexpr const char* RUTAS_MODELOS_LABERINTO_JADE_3D[] =
{
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/losa_marco.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/banda_jugador.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/muro_bloque.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/glifo_muro.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/agujero.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/marca_salida.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/checkpoint.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/altar.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/esfera_jade.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/flecha_trampa.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/boquilla_trampa.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/templo_fondo.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/columna_templo.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/antorcha.glb",
    "Assets/Modelos/Escenarios/Laberinto_Jade/GLB/llama.glb"
};

static_assert(sizeof(RUTAS_MODELOS_LABERINTO_JADE_3D) /
    sizeof(RUTAS_MODELOS_LABERINTO_JADE_3D[0]) == CANTIDAD_MODELOS_LABERINTO_JADE_3D,
    "Las rutas deben coincidir con los IDs del paquete Laberinto Jade");


//==================================================
// Un solo recurso por pieza de la mina, compartido entre geodas y gemas.
// Los IDs siguen el orden de manifest.json del paquete original.
//==================================================

enum ModeloVetaCristal3D
{
    MODELO_VETA_SUELO,
    MODELO_VETA_PAREDES,
    MODELO_VETA_MINERAL_PARED,
    MODELO_VETA_PORTICO,
    MODELO_VETA_POSTE,
    MODELO_VETA_LAMPARA,
    MODELO_VETA_RIEL,
    MODELO_VETA_VAGONETA,
    MODELO_VETA_GEODA_PEQUENA,
    MODELO_VETA_GEODA_GRANDE,
    MODELO_VETA_GEODA_AGOTADA,
    MODELO_VETA_GEMA_AZUL,
    MODELO_VETA_GEMA_DORADA,
    MODELO_VETA_GEMA_VIOLETA,
    MODELO_VETA_MARCA,
    CANTIDAD_MODELOS_VETA_CRISTAL_3D
};

inline constexpr const char* RUTAS_MODELOS_VETA_CRISTAL_3D[] =
{
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/suelo_mina.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/paredes_tunel.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/veta_pared.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/portico_madera.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/poste_lateral.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/lampara_colgante.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/riel_central.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/vagoneta.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/geoda_pequena.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/geoda_grande.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/geoda_agotada.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/gema_azul.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/gema_dorada.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/gema_violeta.glb",
    "Assets/Modelos/Escenarios/Veta_de_Cristal/GLB/marca_geoda.glb"
};

static_assert(sizeof(RUTAS_MODELOS_VETA_CRISTAL_3D) /
    sizeof(RUTAS_MODELOS_VETA_CRISTAL_3D[0]) == CANTIDAD_MODELOS_VETA_CRISTAL_3D,
    "Las rutas deben coincidir con los IDs del paquete Veta de Cristal");


//==================================================
// PAQUETE DE CAPSULAS BARAJADAS
//==================================================

// Cada ID del laboratorio identifica una malla compartida, no una instancia.
enum ModeloCapsulasBarajadas3D
{
    MODELO_CAPSULAS_SALA,
    MODELO_CAPSULAS_MONITOR,
    MODELO_CAPSULAS_MOSTRADOR,
    MODELO_CAPSULAS_TUBO,
    MODELO_CAPSULAS_BALIZA,
    MODELO_CAPSULAS_MESA,
    MODELO_CAPSULAS_PASARELA,
    MODELO_CAPSULAS_BRAZO_BASE,
    MODELO_CAPSULAS_BRAZO_SEGMENTO,
    MODELO_CAPSULAS_BRAZO_ARTICULACION,
    MODELO_CAPSULAS_BRAZO_PINZA,
    MODELO_CAPSULAS_CUERPO,
    MODELO_CAPSULAS_BANDA,
    MODELO_CAPSULAS_TAPA,
    MODELO_CAPSULAS_NUCLEO,
    MODELO_CAPSULAS_MARCADOR,
    CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D
};

inline constexpr const char* RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D[] =
{
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/sala_laboratorio.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/monitor.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/mostrador.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/tubo_ensayo.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/baliza.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/mesa_acero.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/pasarela.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/brazo_base.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/brazo_segmento.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/brazo_articulacion.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/brazo_pinza.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/capsula_cuerpo.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/capsula_banda.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/capsula_tapa.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/nucleo.glb",
    "Assets/Modelos/Escenarios/Capsulas_Barajadas/GLB/marcador.glb"
};

static_assert(sizeof(RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D) /
    sizeof(RUTAS_MODELOS_CAPSULAS_BARAJADAS_3D[0]) == CANTIDAD_MODELOS_CAPSULAS_BARAJADAS_3D,
    "Las rutas deben coincidir con los IDs del paquete Capsulas Barajadas");


//==================================================
// PAQUETE DE BATEO METEORICO
//==================================================

enum ModeloBateoMeteorico3D
{
    MODELO_BATEO_CUMBRE,
    MODELO_BATEO_OBSERVATORIO,
    MODELO_BATEO_TELESCOPIO,
    MODELO_BATEO_OBSERVATORIO_SECUNDARIO,
    MODELO_BATEO_PLANETA,
    MODELO_BATEO_FAROL,
    MODELO_BATEO_CARRIL,
    MODELO_BATEO_CAMPO,
    MODELO_BATEO_CANON_BASE,
    MODELO_BATEO_CANON_TUBO,
    MODELO_BATEO_METEORITO_NORMAL,
    MODELO_BATEO_METEORITO_DORADO,
    MODELO_BATEO_METEORITO_ROJO,
    MODELO_BATEO_BATE,
    MODELO_BATEO_ARO,
    CANTIDAD_MODELOS_BATEO_METEORICO_3D
};

inline constexpr const char* RUTAS_MODELOS_BATEO_METEORICO_3D[] =
{
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/cumbre.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/observatorio_cupula.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/telescopio.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/observatorio_secundario.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/planeta_anillado.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/farol.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/carril_plataforma.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/campo_puntaje.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/canon_base.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/canon_tubo.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/meteorito_normal.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/meteorito_dorado.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/meteorito_rojo.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/bate.glb",
    "Assets/Modelos/Escenarios/Bateo_Meteorico/GLB/aro_punto_dulce.glb"
};

static_assert(sizeof(RUTAS_MODELOS_BATEO_METEORICO_3D) /
    sizeof(RUTAS_MODELOS_BATEO_METEORICO_3D[0]) == CANTIDAD_MODELOS_BATEO_METEORICO_3D,
    "Las rutas deben coincidir con los IDs del paquete Bateo Meteorico");


//==================================================
// PAQUETE DE RACIMO TOXICO
//==================================================

enum ModeloRacimoToxico3D
{
    MODELO_RACIMO_AGUA,
    MODELO_RACIMO_ARBOL_PODRIDO,
    MODELO_RACIMO_ENREDADERA,
    MODELO_RACIMO_FRUTO_NORMAL,
    MODELO_RACIMO_FRUTO_TOXICO,
    MODELO_RACIMO_FRUTO_DORADO,
    MODELO_RACIMO_BALSA,
    MODELO_RACIMO_TRONCO,
    MODELO_RACIMO_CABANA,
    MODELO_RACIMO_ARBOL_FONDO,
    MODELO_RACIMO_LUCIERNAGA,
    MODELO_RACIMO_NENUFAR,
    MODELO_RACIMO_ROCA,
    MODELO_RACIMO_ARO,
    CANTIDAD_MODELOS_RACIMO_TOXICO_3D
};

inline constexpr const char* RUTAS_MODELOS_RACIMO_TOXICO_3D[] =
{
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/agua_pantano.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/arbol_podrido.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/enredadera_central.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/fruto_normal.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/fruto_toxico.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/fruto_dorado.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/balsa.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/tronco_flotante.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/cabana_pilotes.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/arbol_fondo.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/luciernaga.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/nenufar.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/roca_pantano.glb",
    "Assets/Modelos/Escenarios/Racimo_Toxico/GLB/aro_turno.glb"
};

static_assert(sizeof(RUTAS_MODELOS_RACIMO_TOXICO_3D) /
    sizeof(RUTAS_MODELOS_RACIMO_TOXICO_3D[0]) == CANTIDAD_MODELOS_RACIMO_TOXICO_3D,
    "Las rutas deben coincidir con los IDs del paquete Racimo Toxico");


//==================================================
// PAQUETE DE TESORERO CERCADO (minijuego Tesorero Acorralado)
//==================================================

enum ModeloTesoreroCercado3D
{
    MODELO_TESORERO_FOSO,
    MODELO_TESORERO_SUELO,
    MODELO_TESORERO_MURO_FONDO,
    MODELO_TESORERO_MURO_LATERAL,
    MODELO_TESORERO_PARAPETO,
    MODELO_TESORERO_TORRE_ALTA,
    MODELO_TESORERO_TORRE_BAJA,
    MODELO_TESORERO_HOMENAJE,
    MODELO_TESORERO_PORTAL,
    MODELO_TESORERO_ESTANDARTE_ROJO,
    MODELO_TESORERO_ESTANDARTE_DORADO,
    MODELO_TESORERO_ANTORCHA,
    MODELO_TESORERO_MARCO_REJA,
    MODELO_TESORERO_REJA,
    MODELO_TESORERO_AVISO,
    MODELO_TESORERO_MONEDA,
    CANTIDAD_MODELOS_TESORERO_CERCADO_3D
};

inline constexpr const char* RUTAS_MODELOS_TESORERO_CERCADO_3D[] =
{
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/foso_agua.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/suelo_losas.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/muro_fondo.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/muro_lateral.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/parapeto_frontal.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/torre_esquina_alta.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/torre_esquina_baja.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/torre_homenaje.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/portal_fondo.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/estandarte_rojo.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/estandarte_dorado.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/antorcha.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/marco_reja.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/reja_levadiza.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/aviso_reja.glb",
    "Assets/Modelos/Escenarios/Tesorero_Cercado/GLB/moneda_tesoro.glb"
};

static_assert(sizeof(RUTAS_MODELOS_TESORERO_CERCADO_3D) /
    sizeof(RUTAS_MODELOS_TESORERO_CERCADO_3D[0]) == CANTIDAD_MODELOS_TESORERO_CERCADO_3D,
    "Las rutas deben coincidir con los IDs del paquete Tesorero Cercado");


//==================================================
// PAQUETE DE DESCENSO EN NUBES
//==================================================

enum ModeloDescensoNubes3D
{
    MODELO_NUBES_PLANEADOR,
    MODELO_NUBES_PLANEADOR_FRENADO,
    MODELO_NUBES_ANILLO_BLANCO,
    MODELO_NUBES_ANILLO_DORADO,
    MODELO_NUBES_ESTRELLA,
    MODELO_NUBES_NUBE_BLANCA,
    MODELO_NUBES_TORMENTA,
    MODELO_NUBES_FLECHA,
    MODELO_NUBES_BANDA,
    MODELO_NUBES_MAR,
    MODELO_NUBES_ISLA,
    MODELO_NUBES_DIANA,
    MODELO_NUBES_ISLA_FLOTANTE,
    MODELO_NUBES_GLOBO_AZUL,
    MODELO_NUBES_GLOBO_ROJO,
    MODELO_NUBES_MOLINO,
    MODELO_NUBES_ASPAS,
    MODELO_NUBES_AVE,
    MODELO_NUBES_ARCOIRIS,
    CANTIDAD_MODELOS_DESCENSO_NUBES_3D
};

inline constexpr const char* RUTAS_MODELOS_DESCENSO_NUBES_3D[] =
{
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/planeador.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/planeador_frenado.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/anillo_blanco.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/anillo_dorado.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/estrella.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/nube_blanca.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/nube_tormenta.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/flecha_viento.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/banda_viento.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/mar_de_nubes.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/isla_principal.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/diana_aterrizaje.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/isla_flotante.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/globo_azul.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/globo_rojo.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/molino_torre.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/aspas_molino.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/ave.glb",
    "Assets/Modelos/Escenarios/Descenso_en_Nubes/GLB/arcoiris.glb"
};

static_assert(sizeof(RUTAS_MODELOS_DESCENSO_NUBES_3D) /
    sizeof(RUTAS_MODELOS_DESCENSO_NUBES_3D[0]) == CANTIDAD_MODELOS_DESCENSO_NUBES_3D,
    "Las rutas deben coincidir con los IDs del paquete Descenso en Nubes");

//==================================================
// PAQUETE DE VOLEA DE MAGMA
//==================================================

enum ModeloVoleaMagma3D
{
    MODELO_VOLEA_LAGO,
    MODELO_VOLEA_CANCHA,
    MODELO_VOLEA_BORDE,
    MODELO_VOLEA_POSTE,
    MODELO_VOLEA_RED,
    MODELO_VOLEA_VOLCAN_MENOR,
    MODELO_VOLEA_VOLCAN_MAYOR,
    MODELO_VOLEA_COLUMNA,
    MODELO_VOLEA_COLUMNA_LLAMA,
    MODELO_VOLEA_ROCA,
    MODELO_VOLEA_ROCA_CALIENTE,
    MODELO_VOLEA_CHARCO,
    MODELO_VOLEA_SOMBRA,
    MODELO_VOLEA_INDICADOR,
    MODELO_VOLEA_BURBUJA,
    MODELO_VOLEA_CENIZA,
    MODELO_VOLEA_ESTELA,
    CANTIDAD_MODELOS_VOLEA_MAGMA_3D
};

inline constexpr const char* RUTAS_MODELOS_VOLEA_MAGMA_3D[] =
{
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/lago_lava.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/cancha_obsidiana.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/borde_cancha.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/poste_red.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/red_cadenas.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/volcan_menor.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/volcan_mayor.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/columna_basalto.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/columna_con_llama.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/roca_magma.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/roca_sobrecalentada.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/charco_lava.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/sombra_pelota.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/indicador_caida.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/burbuja_lava.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/ceniza.glb",
    "Assets/Modelos/Escenarios/Volea_de_Magma/GLB/estela_ascua.glb"
};

static_assert(sizeof(RUTAS_MODELOS_VOLEA_MAGMA_3D) /
    sizeof(RUTAS_MODELOS_VOLEA_MAGMA_3D[0]) == CANTIDAD_MODELOS_VOLEA_MAGMA_3D,
    "Las rutas deben coincidir con los IDs del paquete Volea de Magma");

//==================================================
// PAQUETE DE PAREJAS GLACIARES
//==================================================

enum ModeloParejasGlaciar3D
{
    MODELO_GLACIAR_MAR,
    MODELO_GLACIAR_LAGO,
    MODELO_GLACIAR_TABLERO,
    MODELO_GLACIAR_BLOQUE_OCULTO,
    MODELO_GLACIAR_BLOQUE_EMPAREJADO,
    MODELO_GLACIAR_CURSOR,
    MODELO_GLACIAR_SIMBOLO_0,
    MODELO_GLACIAR_SIMBOLO_1,
    MODELO_GLACIAR_SIMBOLO_2,
    MODELO_GLACIAR_SIMBOLO_3,
    MODELO_GLACIAR_SIMBOLO_4,
    MODELO_GLACIAR_SIMBOLO_5,
    MODELO_GLACIAR_SIMBOLO_6,
    MODELO_GLACIAR_SIMBOLO_7,
    MODELO_GLACIAR_TEMPANO,
    MODELO_GLACIAR_ICEBERG,
    MODELO_GLACIAR_MONTANA,
    MODELO_GLACIAR_CUEVA,
    MODELO_GLACIAR_PINGUINO,
    MODELO_GLACIAR_FOCA,
    MODELO_GLACIAR_AURORA_VERDE,
    MODELO_GLACIAR_AURORA_VIOLETA,
    MODELO_GLACIAR_AURORA_CIAN,
    MODELO_GLACIAR_FRAGMENTO,
    CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D
};

inline constexpr const char* RUTAS_MODELOS_PAREJAS_GLACIAR_3D[] =
{
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/mar_frio.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/lago_helado.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/tablero_4x4.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/bloque_oculto.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/bloque_emparejado.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/cursor_seleccion.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_0_esfera.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_1_cubo.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_2_cono.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_3_cilindro.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_4_rombo.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_5_nieve.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_6_copo.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/simbolo_7_aurora.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/tempano_jugador.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/iceberg.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/montana_nevada.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/cueva_hielo.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/pinguino.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/foca.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/aurora_verde.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/aurora_violeta.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/aurora_cian.glb",
    "Assets/Modelos/Escenarios/Parejas Glaciares Modelos/GLB/fragmento_hielo.glb"
};

static_assert(sizeof(RUTAS_MODELOS_PAREJAS_GLACIAR_3D) /
    sizeof(RUTAS_MODELOS_PAREJAS_GLACIAR_3D[0]) == CANTIDAD_MODELOS_PAREJAS_GLACIAR_3D,
    "Las rutas deben coincidir con los IDs del paquete Parejas Glaciares");

//==================================================
// PAQUETE DE ESFERAS DEL CANON
//==================================================

enum ModeloEsferasCanon3D
{
    MODELO_ESFERAS_PISTA_1, MODELO_ESFERAS_PARED_IZQUIERDA_1, MODELO_ESFERAS_PARED_DERECHA_1,
    MODELO_ESFERAS_PISTA_2, MODELO_ESFERAS_PARED_IZQUIERDA_2, MODELO_ESFERAS_PARED_DERECHA_2,
    MODELO_ESFERAS_PISTA_3, MODELO_ESFERAS_PARED_IZQUIERDA_3, MODELO_ESFERAS_PARED_DERECHA_3,
    MODELO_ESFERAS_PISTA_4, MODELO_ESFERAS_PARED_IZQUIERDA_4, MODELO_ESFERAS_PARED_DERECHA_4,
    MODELO_ESFERAS_PUENTE_1, MODELO_ESFERAS_PUENTE_2, MODELO_ESFERAS_PUENTE_3,
    MODELO_ESFERAS_RAMPA, MODELO_ESFERAS_ARCO_NATURAL, MODELO_ESFERAS_MESA,
    MODELO_ESFERAS_CACTUS, MODELO_ESFERAS_ROCA, MODELO_ESFERAS_CHECKPOINT,
    MODELO_ESFERAS_SALIDA, MODELO_ESFERAS_META, MODELO_ESFERAS_ESFERA,
    MODELO_ESFERAS_ARO, MODELO_ESFERAS_SUELO,
    CANTIDAD_MODELOS_ESFERAS_CANON_3D
};

inline constexpr const char* RUTAS_MODELOS_ESFERAS_CANON_3D[] =
{
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/tramo_pista_1.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_izquierda_1.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_derecha_1.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/tramo_pista_2.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_izquierda_2.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_derecha_2.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/tramo_pista_3.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_izquierda_3.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_derecha_3.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/tramo_pista_4.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_izquierda_4.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/pared_derecha_4.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/puente_roto_1.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/puente_roto_2.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/puente_roto_3.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/rampa_atajo.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/arco_natural.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/mesa_lejana.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/cactus.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/roca_caida.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/banderin_checkpoint.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/arco_salida.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/arco_meta.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/esfera_piedra.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/aro_jugador.glb",
    "Assets/Modelos/Escenarios/Esferas_del_Canon/GLB/suelo_desertico.glb"
};

static_assert(sizeof(RUTAS_MODELOS_ESFERAS_CANON_3D) /
    sizeof(RUTAS_MODELOS_ESFERAS_CANON_3D[0]) == CANTIDAD_MODELOS_ESFERAS_CANON_3D,
    "Las rutas deben coincidir con los IDs del paquete Esferas del Canon");

//==================================================
// TEXTURAS GENERALES
//==================================================

inline constexpr const char* RUTA_TEXTURA_PISO =
    "Assets/Texturas/Piso.png";

inline constexpr const char* RUTA_TEXTURA_ORO =
    "Assets/Texturas/Oro.png";

inline constexpr const char* RUTA_TEXTURA_ACERO =
    "Assets/Texturas/Acero.png";

inline constexpr const char* RUTA_TEXTURA_JUGADOR =
    "Assets/Texturas/Jugador.png";


//==================================================
// UI
//==================================================

inline constexpr const char* RUTA_LOGO_CREADOR =
    "Assets/UI/LogoCreador.png";

// El menu ya no usa fondos GIF: el HUB 3D (UI/Hub3D.*) es su fondo.

//==================================================
// AUDIO
//==================================================

inline constexpr const char* RUTA_MUSICA_MENU =
    "Assets/Audio/Musica/musicaMenu.mp3";

// Pistas opcionales por categoria (base sin extension; se prueba .ogg y .mp3).
// Si faltan, se mantiene la pista que ya suena.
inline constexpr const char* RUTA_MUSICA_TABLERO = "Assets/Audio/Musica/tablero";
inline constexpr const char* RUTA_MUSICA_MINIJUEGO = "Assets/Audio/Musica/minijuego";
inline constexpr const char* RUTA_MUSICA_RESULTADO = "Assets/Audio/Musica/resultado";

// Efectos de sonido: cada ruta es una base SIN extension. Systems/Audio.cpp
// prueba, en este orden, ".wav", ".ogg" y ".mp3". Si no existe ninguno, el
// juego continua sin ese sonido y avisa en la consola.

// Rutas historicas (raiz de SFX); se conservan para no romper archivos ya
// colocados por el usuario.
inline constexpr const char* RUTA_SFX_UI_MOVER = "Assets/Audio/SFX/ui_mover";
inline constexpr const char* RUTA_SFX_UI_CONFIRMAR = "Assets/Audio/SFX/ui_confirmar";
inline constexpr const char* RUTA_SFX_CUENTA_REGRESIVA = "Assets/Audio/SFX/cuenta_regresiva";
inline constexpr const char* RUTA_SFX_INICIO_MINIJUEGO = "Assets/Audio/SFX/inicio_minijuego";
inline constexpr const char* RUTA_SFX_RECOGER_NUCLEO = "Assets/Audio/SFX/recoger_nucleo";
inline constexpr const char* RUTA_SFX_RECOGER_NUCLEO_ESPECIAL = "Assets/Audio/SFX/recoger_nucleo_especial";
inline constexpr const char* RUTA_SFX_ALERTA_TIEMPO = "Assets/Audio/SFX/alerta_tiempo";
inline constexpr const char* RUTA_SFX_RESULTADO = "Assets/Audio/SFX/resultado";

// UI
inline constexpr const char* RUTA_SFX_UI_CANCELAR = "Assets/Audio/SFX/UI/ui_cancelar";

// Jugador
inline constexpr const char* RUTA_SFX_SALTO = "Assets/Audio/SFX/Jugador/salto";
inline constexpr const char* RUTA_SFX_ATERRIZAJE = "Assets/Audio/SFX/Jugador/aterrizaje";
inline constexpr const char* RUTA_SFX_GOLPE = "Assets/Audio/SFX/Jugador/golpe";
inline constexpr const char* RUTA_SFX_CAIDA = "Assets/Audio/SFX/Jugador/caida";
inline constexpr const char* RUTA_SFX_GROUND_POUND = "Assets/Audio/SFX/Jugador/ground_pound";

// Tablero
inline constexpr const char* RUTA_SFX_DADO = "Assets/Audio/SFX/Tablero/dado";
inline constexpr const char* RUTA_SFX_PASO_TABLERO = "Assets/Audio/SFX/Tablero/paso";
inline constexpr const char* RUTA_SFX_MONEDA = "Assets/Audio/SFX/Tablero/moneda";
inline constexpr const char* RUTA_SFX_COMPRA = "Assets/Audio/SFX/Tablero/compra";
inline constexpr const char* RUTA_SFX_CASILLA_POSITIVA = "Assets/Audio/SFX/Tablero/casilla_positiva";
inline constexpr const char* RUTA_SFX_CASILLA_NEGATIVA = "Assets/Audio/SFX/Tablero/casilla_negativa";
inline constexpr const char* RUTA_SFX_EVENTO_TABLERO = "Assets/Audio/SFX/Tablero/evento";
inline constexpr const char* RUTA_SFX_RULETA_TICK = "Assets/Audio/SFX/Tablero/ruleta_tick";

// Minijuegos (eventos comunes reutilizables)
inline constexpr const char* RUTA_SFX_RECOGER_OBJETO = "Assets/Audio/SFX/Minijuegos/recoger";
inline constexpr const char* RUTA_SFX_EXPLOSION = "Assets/Audio/SFX/Minijuegos/explosion";
inline constexpr const char* RUTA_SFX_BOTON = "Assets/Audio/SFX/Minijuegos/boton";
inline constexpr const char* RUTA_SFX_DISPARO = "Assets/Audio/SFX/Minijuegos/disparo";
inline constexpr const char* RUTA_SFX_PLATAFORMA = "Assets/Audio/SFX/Minijuegos/plataforma";
inline constexpr const char* RUTA_SFX_IMPACTO = "Assets/Audio/SFX/Minijuegos/impacto";
inline constexpr const char* RUTA_SFX_ACIERTO = "Assets/Audio/SFX/Minijuegos/acierto";
inline constexpr const char* RUTA_SFX_ERROR = "Assets/Audio/SFX/Minijuegos/error";
inline constexpr const char* RUTA_SFX_ELIMINADO = "Assets/Audio/SFX/Minijuegos/eliminado";


//==================================================
// TEXTURAS OPCIONALES DE MINIJUEGOS
//==================================================

inline constexpr const char* RUTA_TEXTURA_LAVA_SUELO =
    "Assets/Texturas/Minijuegos/ColorSeguro/lava_suelo.png";
inline constexpr const char* RUTA_TEXTURA_LAVA_FONDO =
    "Assets/Texturas/Minijuegos/ColorSeguro/fondo_volcanes.png";

inline constexpr const char* RUTA_TEXTURA_NIEVE_SUELO =
    "Assets/Texturas/Minijuegos/Pelotas/nieve_suelo.png";
inline constexpr const char* RUTA_TEXTURA_NIEVE_FONDO =
    "Assets/Texturas/Minijuegos/Pelotas/fondo_montanas.png";

inline constexpr const char* RUTA_TEXTURA_CUEVA_ROCA =
    "Assets/Texturas/Minijuegos/RefugioTaladros/roca_cueva.png";
inline constexpr const char* RUTA_TEXTURA_CUEVA_TALADRO =
    "Assets/Texturas/Minijuegos/RefugioTaladros/metal_taladro.png";

inline constexpr const char* RUTA_TEXTURA_MAGNETICA_METAL =
    "Assets/Texturas/Minijuegos/TormentaMagnetica/metal_arena.png";
inline constexpr const char* RUTA_TEXTURA_MAGNETICA_ENERGIA =
    "Assets/Texturas/Minijuegos/TormentaMagnetica/energia.png";


//==================================================
// SELECCION DE PERSONAJES
//==================================================

inline constexpr const char* RUTA_ICONO_PERSONAJE_1 =
    "Assets/Personajes/TungTung/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_1 =
    "Assets/Personajes/TungTung/Retrato.png";

inline constexpr const char* RUTA_ICONO_PERSONAJE_2 =
    "Assets/Personajes/Personaje2/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_2 =
    "Assets/Personajes/Personaje2/Retrato.png";

inline constexpr const char* RUTA_ICONO_PERSONAJE_3 =
    "Assets/Personajes/Personaje3/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_3 =
    "Assets/Personajes/Personaje3/Retrato.png";

inline constexpr const char* RUTA_ICONO_PERSONAJE_4 =
    "Assets/Personajes/Personaje4/Icono.png";
inline constexpr const char* RUTA_RETRATO_PERSONAJE_4 =
    "Assets/Personajes/Personaje4/Retrato.png";
