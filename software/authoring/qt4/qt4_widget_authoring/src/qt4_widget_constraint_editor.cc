#include <QtGui/QHBoxLayout>

#include "authoring/qt4_widget_constraint_task_space_region_editor.h"
#include "authoring/qt4_widget_constraint_editor.h"

using namespace std;
using namespace boost;
using namespace urdf;
using namespace affordance;
using namespace authoring;

Qt4_Widget_Constraint_Editor::
Qt4_Widget_Constraint_Editor( Constraint *& constraint,
                              Model& robotModel,
                              vector< AffordanceState >& affordanceCollection,
                              const string& id,
                              QWidget * parent ) : QWidget( parent ),
                                                    _constraint( constraint ),
                                                    _robot_model( robotModel ),
                                                    _robot_affordances(),
                                                    _object_affordances( affordanceCollection ),
                                                    _id( id ),
                                                    _label_id( new QLabel( QString::fromStdString( id ), this ) ),
                                                    _check_box_active( new QCheckBox( this ) ),
                                                    _combo_box_type( new QComboBox( this ) ),
                                                    _push_button_edit( new QPushButton( QString( "edit" ), this ) ),
                                                    _double_spin_box_time_start( new QDoubleSpinBox( this ) ),
                                                    _double_spin_box_time_end( new QDoubleSpinBox( this ) ),
                                                    _constraint_editor_popup( NULL ) {
  vector< shared_ptr< Link > > links;
  _robot_model.getLinks( links );
  for( vector< shared_ptr< Link > >::iterator it1 = links.begin(); it1 != links.end(); it1++ ){
    for( map< string, shared_ptr< vector< shared_ptr< Collision > > > >::iterator it2 = (*it1)->collision_groups.begin(); it2 != (*it1)->collision_groups.end(); it2++ ){
      _robot_affordances.push_back( pair< string, string >( (*it1)->name, it2->first ) );
    }
  }

  for( unsigned int i = 0; i < NUM_CONSTRAINT_TYPES; i++ ){
    _combo_box_type->addItem( QString::fromStdString( Constraint::constraint_type_t_to_std_string( ( constraint_type_t )( i ) ) ) );
  }

  _label_id->setFixedWidth( 35 );
  _check_box_active->setFixedWidth( 25 );
  _combo_box_type->setFixedWidth( 65 );
  _push_button_edit->setFixedWidth( 50 );
  _double_spin_box_time_start->setFixedWidth( 70 );
  _double_spin_box_time_start->setRange( 0.0, 1000000.0 );
  _double_spin_box_time_start->setSingleStep( 0.1 );
  _double_spin_box_time_start->setSuffix( " s" );
  _double_spin_box_time_end->setFixedWidth( 70 );
  _double_spin_box_time_end->setRange( 0.0, 1000000.0 );
  _double_spin_box_time_end->setSingleStep( 0.1 );
  _double_spin_box_time_end->setSuffix( " s" );

  QHBoxLayout * widget_layout = new QHBoxLayout();
  widget_layout->addWidget( _check_box_active );
  widget_layout->addWidget( _label_id );
  widget_layout->addWidget( _combo_box_type );
  widget_layout->addWidget( _push_button_edit );
  widget_layout->addWidget( _double_spin_box_time_start );
  widget_layout->addWidget( _double_spin_box_time_end );
  setLayout( widget_layout );

  connect( _check_box_active, SIGNAL( stateChanged( int ) ), this, SLOT( _check_box_active_changed( int ) ) );
  connect( _combo_box_type, SIGNAL( currentIndexChanged( int ) ), this, SLOT( _combo_box_type_changed( int ) ) );
  connect( _push_button_edit, SIGNAL( clicked() ), this, SLOT( _push_button_edit_pressed() ) );
  connect( _double_spin_box_time_start, SIGNAL( valueChanged( double ) ), this, SLOT( _double_spin_box_time_start_value_changed( double ) ) );
  connect( _double_spin_box_time_end, SIGNAL( valueChanged( double ) ), this, SLOT( _double_spin_box_time_end_value_changed( double ) ) );

  if( constraint != NULL ){
    _combo_box_type->setEnabled( constraint->active() );
    _push_button_edit->setEnabled( constraint->active() );
    _double_spin_box_time_start->setEnabled( constraint->active() );
    _double_spin_box_time_end->setEnabled( constraint->active() );
  } else {
    _combo_box_type->setEnabled( false );
    _push_button_edit->setEnabled( false );
    _double_spin_box_time_start->setEnabled( false );
    _double_spin_box_time_end->setEnabled( false );
  }
}

Qt4_Widget_Constraint_Editor::
~Qt4_Widget_Constraint_Editor() {

}

Qt4_Widget_Constraint_Editor::
Qt4_Widget_Constraint_Editor( const Qt4_Widget_Constraint_Editor& other ) : QWidget(),
                                                                            _constraint( other._constraint ),
                                                                            _robot_model( other._robot_model ),
                                                                            _object_affordances( other._object_affordances ) {

}

Qt4_Widget_Constraint_Editor&
Qt4_Widget_Constraint_Editor::
operator=( const Qt4_Widget_Constraint_Editor& other ) {

  return (*this);
}

void
Qt4_Widget_Constraint_Editor::
update_constraint( void ){
  if( _constraint != NULL ){
    if( _constraint->active() ){
      _check_box_active->setCheckState( Qt::Checked );
    } else {
      _check_box_active->setCheckState( Qt::Unchecked );
    }
    _combo_box_type->setCurrentIndex( _constraint->type() );
    _double_spin_box_time_start->setValue( _constraint->start() );
    _double_spin_box_time_end->setValue( _constraint->end() );
  } else {
    _check_box_active->setCheckState( Qt::Unchecked );
  }
  return;
}

void
Qt4_Widget_Constraint_Editor::
_double_spin_box_time_start_value_changed( double start ){
  if( _constraint != NULL ){
    _constraint->start() = _double_spin_box_time_start->value();
  }
  return;
}

void
Qt4_Widget_Constraint_Editor::
_double_spin_box_time_end_value_changed( double end ){
  if( _constraint != NULL ){
    _constraint->end() = _double_spin_box_time_end->value();
  }
  return;
}

void
Qt4_Widget_Constraint_Editor::
_check_box_active_changed( int state ){
  switch( _check_box_active->checkState() ){
  case ( Qt::Unchecked ):
  case ( Qt::PartiallyChecked ):
    _combo_box_type->setEnabled( false );
    _push_button_edit->setEnabled( false );
    _double_spin_box_time_start->setEnabled( false );
    _double_spin_box_time_end->setEnabled( false );
    if( _constraint_editor_popup != NULL ){
      _constraint_editor_popup->close(); 
      delete _constraint_editor_popup;
      _constraint_editor_popup = NULL;
    }
    if( _constraint != NULL ){
      emit info_update( QString( "[<b>OK</b>] deactivating constraint %1" ).arg( QString::fromStdString( _constraint->id() ) ) );
      _constraint->active() = false;
    } else {
      emit info_update( QString( "[<b>OK</b>] deactivating constraint %1" ).arg( QString::fromStdString( _id ) ) );
    }
    break;
  case ( Qt::Checked ):
    _combo_box_type->setEnabled( true );
    if( _constraint != NULL ){
      _constraint->active() = true;
      if( _constraint->type() == _combo_box_type->currentIndex() ){
        _push_button_edit->setEnabled( true );
        _double_spin_box_time_start->setEnabled( true );
        _double_spin_box_time_end->setEnabled( true );
      }
      emit info_update( QString( "[<b>OK</b>] activating constraint %1" ).arg( QString::fromStdString( _constraint->id() ) ) );
    } else {
      emit info_update( QString( "[<b>OK</b>] activating constraint %1" ).arg( QString::fromStdString( _id ) ) );
    }
    break;
  default:
    break;
  }
  return;
}

void
Qt4_Widget_Constraint_Editor::
_combo_box_type_changed( int index ){
  if( _constraint != NULL ){
    if( _constraint->type() != _combo_box_type->currentIndex() ){
      delete _constraint;
      _constraint = NULL;
      emit info_update( QString( "[<b>OK</b>] deleted constraint %1" ).arg( QString::fromStdString( _id ) ) );
      switch( _combo_box_type->currentIndex() ){
      case ( CONSTRAINT_TASK_SPACE_REGION_TYPE ):
        _constraint = new Constraint_Task_Space_Region( _id );
        emit info_update( QString( "[<b>OK</b>] instatiated new task space region constraint %1" ).arg( QString::fromStdString( _constraint->id() ) ) );
        break;
      case ( CONSTRAINT_CONFIGURATION_TYPE ):
        break;
      case ( CONSTRAINT_UNKNOWN_TYPE ):
      default:
        break;
      }
    }    
  } else {
    switch( _combo_box_type->currentIndex() ){
    case ( CONSTRAINT_TASK_SPACE_REGION_TYPE ):
      _constraint = new Constraint_Task_Space_Region( _id );
      emit info_update( QString( "[<b>OK</b>] instatiated new task space region constraint %1" ).arg( QString::fromStdString( _constraint->id() ) ) );
      break;
    case ( CONSTRAINT_CONFIGURATION_TYPE ):
      break;
    case ( CONSTRAINT_UNKNOWN_TYPE ):
    default:
      break;
    }
  }

  switch( _combo_box_type->currentIndex() ){
  case ( CONSTRAINT_TASK_SPACE_REGION_TYPE ):
    _push_button_edit->setEnabled( true );
    _double_spin_box_time_start->setEnabled( true );
    _double_spin_box_time_end->setEnabled( true );
    break;
  case ( CONSTRAINT_CONFIGURATION_TYPE ):
    _push_button_edit->setEnabled( true );
    _double_spin_box_time_start->setEnabled( true );
    _double_spin_box_time_end->setEnabled( true );
    break;
  case ( CONSTRAINT_UNKNOWN_TYPE ):
  default:
    _push_button_edit->setEnabled( false );
    _double_spin_box_time_start->setEnabled( false );
    _double_spin_box_time_end->setEnabled( false );
    break;
  }

  return;
}

void
Qt4_Widget_Constraint_Editor::
_push_button_edit_pressed( void ){
  if( _constraint != NULL ){
    switch( _constraint->type() ){
    case ( CONSTRAINT_TASK_SPACE_REGION_TYPE ):
      _constraint_editor_popup = new Qt4_Widget_Constraint_Task_Space_Region_Editor( dynamic_cast< Constraint_Task_Space_Region* >( _constraint ), _robot_model, _object_affordances, this );
      _constraint_editor_popup->show();
      emit info_update( QString( "[<b>OK</b>] launching editor for constraint %1" ).arg( QString::fromStdString( _constraint->id() ) ) );
      break;
    case ( CONSTRAINT_CONFIGURATION_TYPE ):
      break;
    case ( CONSTRAINT_UNKNOWN_TYPE ):
    default:
      break;
    }

  } else {
    emit info_update( QString( "[<b>ERROR</b>] cannot edit UNKNOWN constraint type" ) );
  }
  return;
}

namespace authoring {
  ostream&
  operator<<( ostream& out,
              const Qt4_Widget_Constraint_Editor& other ) {
    return out;
  }

}
