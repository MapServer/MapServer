/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Perl-specific enhancements to MapScript
 * Author:   SDL based on Sean's Python conventions
 *
 ******************************************************************************
 *
 * Perl-specific mapscript code has been moved into this 
 * SWIG interface file to improve the readibility of the main
 * interface file.  The main mapscript.i file includes this
 * file when SWIGPERL is defined (via 'swig -perl5 ...').
 *
 *****************************************************************************/

/******************************************************************************
 * Simple Typemaps
 *****************************************************************************/

%init %{
  if(msSetup() != MS_SUCCESS) {
    msSetError(MS_MISCERR, "Error initializing MapServer/Mapscript.", "msSetup()");
  }
%}

/* Translate Perl's built-in file object to FILE * */
%typemap(in) FILE * {
  $1 = PerlIO_exportFILE (IoIFP (sv_2io ($input)), NULL);
}

/* To support imageObj::getBytes */
%typemap(out) gdBuffer {
        SV *mysv;
        mysv = sv_newmortal();
        if ($1.data == NULL)
            sv_setpv(mysv,"");
        else
            sv_setpvn(mysv,(const char*)$1.data,$1.size);
        $result = newRV(mysv);
        sv_2mortal($result);
        argvi++;
        if( $1.owns_data )
            msFree($1.data);
}

%typemap(out) char[ANY] {
        $result = newSVpvn($1, strlen($1));
        argvi++;
}

/*
===============================================================================
RFC-24 implementation follows
===============================================================================
   Modified constructor according to:
   - cache population and sync, item 3.2
   - reference counter since Perl does not obey %newobject
*/
%allowexception;

%exception layer {
	/* Accessing layer */
	$action;
	MS_REFCNT_INCR(result);
}
%exception map {
	/* Accessing map */
	$action;
	MS_REFCNT_INCR(result);
}

%feature("shadow") layerObj(mapObj *map)
%{
sub new {
        my $pkg = shift;
        my $self = mapscriptc::new_layerObj(@_);
        bless $self, $pkg if defined($self);
        if (defined($_[0])) {
                # parent reference
                mapscript::RFC24_ADD_PARENT_REF( tied(%$self), $_[0]);
        }
        return $self;
}
%}

%feature("shadow") getLayer(int i)
%{
sub getLayer {
        my $layer = mapscriptc::mapObj_getLayer(@_);
        if (defined($layer)) {
                # parent reference
                #print "l=$layer,m=$_[0]\n";
                mapscript::RFC24_ADD_PARENT_REF( tied(%$layer) , $_[0]);
        }
        return $layer;
}
%}

%feature("shadow") getLayerByName(char* name)
%{
sub getLayerByName {
        my $layer = mapscriptc::mapObj_getLayerByName(@_);
        if (defined($layer)) {
                # parent reference
                #print "l=$layer,m=$_[0]\n";
                mapscript::RFC24_ADD_PARENT_REF( tied(%$layer) , $_[0]);
        }
        return $layer;
}
%}

%feature("shadow") insertLayer
%{
sub insertLayer {
        my $idx = mapscriptc::mapObj_insertLayer(@_);
        my $layer=$_[1];
        # parent reference
        mapscript::RFC24_ADD_PARENT_REF(tied(%$layer), $_[0]);
        return $idx;
}
%}

%feature("shadow") ~layerObj()
%{
sub DESTROY {
        return unless $_[0]->isa('HASH');
        my $self = tied(%{$_[0]});
        return unless defined $self;
        delete $ITERATORS{$self};
        mapscriptc::delete_layerObj($self);
        # remove parent reference
        mapscript::RFC24_DEL_PARENT_REF($self);
}
%}

%feature("shadow") classObj(layerObj *layer)
%{
sub new {
        my $pkg = shift;
        my $self = mapscriptc::new_classObj(@_);
        bless $self, $pkg if defined($self);
        if (defined($_[0])) {
                # parent reference
                mapscript::RFC24_ADD_PARENT_REF( tied(%$self), $_[0]);
        }
        return $self;
}
%}

%feature("shadow") getClass(int i)
%{
sub getClass {
        my $clazz = mapscriptc::layerObj_getClass(@_);
        if (defined($clazz)) {
                # parent reference
                mapscript::RFC24_ADD_PARENT_REF( tied(%$clazz) , $_[0]);
        }
        return $clazz;
}
%}
%feature("shadow") insertClass
%{
sub insertClass {
        my $idx = mapscriptc::layerObj_insertClass(@_);
        my $clazz=$_[1];
        # parent reference
        mapscript::RFC24_ADD_PARENT_REF(tied(%$clazz), $_[0]);
        return $idx;
}
%}

%feature("shadow") ~classObj()
%{
sub DESTROY {
        return unless $_[0]->isa('HASH');
        my $self = tied(%{$_[0]});
        return unless defined $self;
        delete $ITERATORS{$self};
        mapscriptc::delete_classObj($self);
        # remove parent reference
        mapscript::RFC24_DEL_PARENT_REF($self);
}
%}
%perlcode %{
        %PARENT_PTRS=();
        sub RFC24_ADD_PARENT_REF {
                my ($child, $parent)=@_;
                $PARENT_PTRS{ $$child }=$parent;
        }
        sub RFC24_DEL_PARENT_REF {
                my ($child)=@_;
                delete $PARENT_PTRS{ $$child };
        }

        # USE THIS function only for debugging!
        sub getPARENT_PTRS {
                return \%PARENT_PTRS;
        }
%}
