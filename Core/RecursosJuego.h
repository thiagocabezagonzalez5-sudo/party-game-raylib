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
