class_name Saeule
extends Node3D
## Eine Marke in der Welt: ein flacher Ring am Boden und eine hohe, durchscheinende
## Saeule, die man ueber die Daecher sieht. Wegmarken (Auftrag, Laden) sind reine
## Zeichen; Revier-Saeulen sind treffbar, damit Farbkugeln sie markieren koennen.

var _ring := MeshInstance3D.new()
var _pfeiler := MeshInstance3D.new()
var _ring_stoff := StandardMaterial3D.new()
var _pfeiler_stoff := StandardMaterial3D.new()
var _koerper: StaticBody3D


## ring: Durchmesser des Rings in Metern; hoehe und dicke: die Saeule; versatz:
## seitlich vom Ring; treffbar: bekommt eine Kollisionsform.
static func neu(farbe: Color, ring: float, hoehe: float, dicke: float,
		versatz := Vector3.ZERO, treffbar := false) -> Saeule:
	var s := Saeule.new()
	s._baue(farbe, ring, hoehe, dicke, versatz, treffbar)
	return s


func _baue(farbe: Color, ring: float, hoehe: float, dicke: float, versatz: Vector3,
		treffbar: bool) -> void:
	var zyl := CylinderMesh.new()
	zyl.top_radius = ring / 2.0
	zyl.bottom_radius = ring / 2.0
	zyl.height = 0.08
	_ring.mesh = zyl
	_ring.position.y = 0.12
	_ring_stoff.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_ring.material_override = _ring_stoff
	_ring.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(_ring)

	var pf := CylinderMesh.new()
	pf.top_radius = dicke / 2.0
	pf.bottom_radius = dicke / 2.0
	pf.height = hoehe
	_pfeiler.mesh = pf
	_pfeiler.position = versatz + Vector3(0, hoehe / 2.0, 0)
	_pfeiler_stoff.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_pfeiler_stoff.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_pfeiler.material_override = _pfeiler_stoff
	_pfeiler.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(_pfeiler)
	setze_farbe(farbe)

	if treffbar:
		_koerper = StaticBody3D.new()
		var form := CollisionShape3D.new()
		var zs := CylinderShape3D.new()
		zs.radius = dicke / 2.0
		zs.height = hoehe
		form.shape = zs
		_koerper.add_child(form)
		_koerper.position = _pfeiler.position
		add_child(_koerper)


func setze_farbe(farbe: Color) -> void:
	_ring_stoff.albedo_color = farbe
	_pfeiler_stoff.albedo_color = Color(farbe.r, farbe.g, farbe.b, 0.35)


## Der Koerper, den eine Farbkugel trifft; null bei einer reinen Wegmarke.
func koerper() -> StaticBody3D:
	return _koerper


func zeige(an: bool) -> void:
	visible = an
	if _koerper != null:
		_koerper.collision_layer = 1 if an else 0
